/*
 * (C) Copyright 1996-2012 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <random>
#include <cmath>

#include "eckit/log/Log.h"
#include "eckit/runtime/Tool.h"
#include "eckit/runtime/Context.h"
#include "eckit/filesystem/FileHandle.h"


#include "eckit/grib/GribFieldSet.h"
#include "eckit/grib/GribField.h"
#include "eckit/grib/GribCompute.h"

#include "eckit/utils/Timer.h"

#include "eckit/container/BSPTree.h"

#include "eckit/mining/Tools.h"

#include "eckit/mining/SmallestSphere.h"



using namespace eckit;
using namespace eckit::compute;

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

class Compute : public Tool {
public:

    Compute(int argc,char **argv): Tool(argc,argv) {
    }

    ~Compute() {
    }

    void kmeans(GribFieldSet&);
    void kernel(GribFieldSet&);
    void dbscan(GribFieldSet&);
    void bsptree(GribFieldSet&);

    virtual void run();
};

//-----------------------------------------------------------------------------


GribFieldSet normalise(const GribFieldSet& s) {
    //return normalise2(s);
    return s;
}

double distance(const GribFieldSet& a, const GribFieldSet& b)
{
    GribFieldSet c = a-b;
    return std::sqrt(compute::accumulate(c*c));
}

void Compute::kernel(GribFieldSet & members)
{
    Matrix k = compute::kernel(members);
    Log::info() << k << endl;
}

void Compute::kmeans(GribFieldSet & fields)
{
    int K = 12;


    vector<vector<size_t> > last;
    vector<GribFieldSet> centroids(K);
    std::default_random_engine generator;
    std::uniform_int_distribution<size_t> distribution(0, fields.size()-1);
    std::set<size_t> seen;

    for(size_t i = 0; i < K; i++) {
        size_t choice = distribution(generator);
        while(seen.find(choice) != seen.end()) {
            choice = distribution(generator);
        }
        seen.insert(choice);
        centroids[i] = fields[choice];
    }

    Timer timer("k-mean");

    for(;;)
    {
        vector<vector<size_t> > ranks(K);

        for(size_t j = 0 ; j < fields.size(); ++j)
        {
            GribFieldSet m = fields[j];

            double min_distance = ::distance(m, centroids[0]);
            int    min_index    = 0;
            for(size_t i = 1; i < K; ++i)
            {
                double x = ::distance(m, centroids[i]);
                if(x < min_distance) {
                    min_distance = x;
                    min_index    = i;
                }
            }

            ranks[min_index].push_back(j);

        }

        for(size_t i = 0; i < K; i++) {
            cout << ranks[i].size() << ' ';
        }
        cout << endl;

        for(size_t i = 0; i < K; i++) {
            centroids[i] = mean(fields.slice(ranks[i]));
        }

        if (ranks == last)
            break;

        last = ranks;
    }

    for(size_t i = 0; i < K; i++) {
        GribFieldSet members = fields.slice(last[i]);
        double max  = 0;
        for(size_t j = 0 ; j < members.size(); ++j) {
            double d = ::distance(members[j], centroids[i]);
            if(d > max) {
                max = d;
            }
        }
        cout << i << " " << max << endl;
    }

    GribFieldSet result(centroids);
    result.write("/tmp/centroids.grid");
}


void Compute::dbscan(GribFieldSet & members)
{
    // Page 417

    double epsilon = 12.0;
    size_t minpts = 1000;

    Timer timer("dbscan");


    for(size_t i = 0 ; i < members.size(); ++i)
    {
        GribFieldSet n = members[i];
        int N = 0;

        for(size_t j = 0 ; j < members.size(); ++j)
        {

            GribFieldSet m = members[j];
            if(::distance(m, n) < epsilon) {
                N++;
            }

        }

        if(N > minpts) {
            cout << i << " " << N << endl;
        }
    }



    //GribFieldSet result(centroids);
    //result.write("/tmp/dbscan.grid");
}

size_t compares = 0;

class BSPWrapper : public GribFieldSet {
    const double* values_;
    size_t count_;

public:

    BSPWrapper(const GribFieldSet& fs):
        GribFieldSet(fs) {
        ASSERT(fs.size() == 1);
        values_ = get(0)->getValues(count_);
    }

    static BSPWrapper mean(const vector<BSPWrapper>& v)
    {
        vector<GribFieldSet> f;
        f.resize(v.size());
        std::copy(v.begin(), v.end(), std::back_inserter(f));
        return compute::mean(GribFieldSet(f));
    }

    static BSPWrapper symetrical(const BSPWrapper& w, const BSPWrapper& c) {
        return BSPWrapper(w-(c-w));
    }

    static double distance(const BSPWrapper& a, const BSPWrapper& b) {
        return ::distance(a, b);

    }


    /*
    friend bool operator==(const BSPWrapper& a, const BSPWrapper& b) {
        const double epsilon = 1e-15;
        if(a.count_ == b.count_ ) {
            for(size_t j = 0; j < a.count_ ; ++j) {
                double x = std::fabs(a.values_[j] - b.values_[j]);
                if(x > epsilon) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
    */

};



void Compute::bsptree(GribFieldSet & members)
{
    Timer timer("bsptree");

    BSPTree<BSPWrapper> tree;
    vector<BSPWrapper> v;

    cout << "copy" << endl;
    std::copy(members.begin(), members.end(), std::back_inserter(v));

    {
        cout << "Build" << endl;
        Timer timer("build");
        tree.build(v);
    }

    if(true) {

        vector<DataHandle*> files;


        struct V {
            void enter(const BSPWrapper& node, bool, size_t depth)  {
                while(depth >= files_.size()) {
                    StrStream os;
                    os << "node_" << depth << ".grib" << StrStream::ends;
                    cout << string(os) << endl;
                    DataHandle* h = new FileHandle(string(os));
                    h->openForWrite(0);
                    files_.push_back(h);
                }
                node.write(*(files_[depth]));

            }
            void leave(const BSPWrapper&, bool, size_t depth)  {
            }

            vector<DataHandle*> files_;

            V(vector<DataHandle*> files):files_(files) {}
        };

        V vv(files);

        tree.visit(vv);


        for(vector<DataHandle*>::const_iterator j =  files.begin(); j != files.end(); ++j) {
            DataHandle *h = *j;
            h->close();
            delete h;
        }
    }

    {
        GribFieldSet f("/tmp/today.grib");
        f = normalise(f);
        f.write("normalised.grib");

        BSPTree<BSPWrapper>::NodeInfo x = tree.nearestNeighbour(f);
        cout << "nearestNeighbour :" << x << endl;
        x.point().write("nearest.grib");
    }

    {
        GribFieldSet today("/tmp/today.grib");
        today = normalise(today);
        BSPTree<BSPWrapper>::NodeInfo x = tree.nearestNeighbourBruteForce(today);
        cout << "nearestNeighbourBruteForce :" << x << endl;
        x.point().write("brute.grib");
    }

#if 0
    size_t i = 0;
    for(vector<BSPWrapper>::const_iterator j = v.begin(); j != v.end(); ++j, ++i) {
        compares = 0;
        BSPTree<BSPWrapper>::NodeList x = tree.findInSphere(*j, 12.0);
        cout << i << ' ' << x.size() << " " << compares << endl;
    }
#endif


    //GribFieldSet result(centroids);
    //result.write("/tmp/dbscan.grid");
}

void Compute::run()
{

    GribFieldSet members("/tmp/data.grib");

    Log::info() << "Before: " << members << endl;
    Log::info() << "MAX: " << maxvalue(members) << endl;
    Log::info() << "MIN: " << minvalue(members) << endl;

    members = normalise(members);

    Log::info() << "After: " << members << endl;
    Log::info() << "MAX: " << maxvalue(members) << endl;
    Log::info() << "MIN: " << minvalue(members) << endl;


    bsptree(members);


}

//-----------------------------------------------------------------------------

int main(int argc,char **argv)
{
    Compute app(argc,argv);
    app.start();
    return 0;
}

