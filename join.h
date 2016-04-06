/*
 * Copyright (c) 2015 Nutanix Inc. All rights reserved.
 *
 * Author: jayendr.gowrish@nutanix.com
 * Purpose: Log parser, general error diagnoser on customer 
 * clusters
*/

#ifndef _CEREBRO_DIAGNOSE_JOIN_
#define _CEREBRO_DIAGNOSE_JOIN_

#include <iostream>
#include <map>


using namespace std;
namespace nutanix { namespace cerebro { namespace diagnose {

template <class L, class R>
class Join {
public:
    Join()
       :left(nullptr), right(nullptr)
    {};
    
    Join(unique_ptr<L> l, unique_ptr<R> r, unique_ptr<RULE> rule)
       :pleft_(l), pright_(r), prule_(rule)
    {
        CHECK_NE(pleft_, nullptr);
        CHECK_NE(pright_, nullptr);
        CHECK_NE(prule_, nullptr);
        // Initialize the results vector
    };

    ~Join(){};

    vector<string> GetResults() const;

private:
    // Join executor
    vector<string> ExecuteJoin() {
        // FOr every entry in left_
        // Get record pair<[key_string, value_string]>
        // Look up key_string on right_, get record pair
        // results_.push_back (RULE(lrecord, rrecord))
        //
        //
    }
    
    // Left table
    unique_ptr<L>    pleft_;

    // Right table
    unique_ptr<R>    pright_;

    // Rule function
    unique_ptr<RULE> prule_; //TODO this should really be a vecror of rules

    //Output: File list for all meta ops
    //select file_name,vdisk_id from join(completed, file_vdiskId,Key = MetaopId)
    //MetaopId: 12345 (DISPLAY KEY regardless)
    //file_name : <fn1> vdisk_id: <id1> (DISPLAY THE TWO SPECIFIED COLS)
    //          : <fn2> vdisk_id: <id2>
    //          : <fn3> vdisk_id: <id3>
    //select 
    //Source_col2 : Name
    //Dest_col1 : Name
    //Dest_col2 : Name
    unordered_set<pair<string,string>> results_;
};

template <class L, class R>
class Equals {
public:
    Equals(){}
    ~Equals(){}

    vector<string> operator(const string& key)()
    {
        // Lookup key in L, add to return vector

        // Lookup key in R, add to return vector

    }
};

} } }// namespace

#endif
