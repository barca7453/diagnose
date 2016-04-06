/*
 * Copyright (c) 2015 Nutanix Inc. All rights reserved.
 *
 * Author: jayendr.gowrish@nutanix.com
 * Purpose: Log parser, general error diagnoser on customer 
 * clusters
*/

#ifndef _CEREBRO_DIAGNOSE_INFOLOG_PARSER_
#define _CEREBRO_DIAGNOSE_INFOLOG_PARSER_

#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

#include <memory>
#include <regex>
#include <sstream> 
#include <iostream>
#include <unordered_set>

using namespace std;
namespace nutanix { namespace cerebro { namespace diagnose {

// All tables are maps of
// key (unique primary key)
// Value : Map of key = attrib name, value = attrib value
// Policies for this? Threading model, locking, allocation
struct Table {
    typedef boost::char_separator<char> CHAR_SEP;
    typedef map<string,string> ATTRIB_MAP;
    typedef pair<string, shared_ptr<ATTRIB_MAP>> ROW; 
    typedef map<string, shared_ptr<ATTRIB_MAP>> TABLE; 

    Table() {}
    virtual ~Table() { } 

    virtual void InsertRecord (const string& key,
       shared_ptr<ATTRIB_MAP> attributes,
       bool replace = false) {
        if (replace){
            table_[key] = attributes;
        } else {
            if (table_.find(key) == end(table_)) {
                table_[key] = attributes;
            }
        }
        schema_attributes_.insert(key);
    }
    
    bool Exists (const string& key) {
        return (GetRecord(key) != nullptr);
    }

    // key = unique key
    // attrib_name = name of the column to be replaced
    // attrib_value = value
    // append = should I replace or append?
    virtual void UpdateRecord (const string& key,
       const string& attrib_name,
       const string& attrib_value,
       bool append = false) {
        if (table_.find(key) == end(table_)) {
            return;
        } 

        string value = (*table_[key])[attrib_name];// Make this a function..GetColValue
        if (append) {
            value += "\n";
            value += attrib_value;
        } else {
            value = attrib_value;
        }
        (*table_[key])[attrib_name] = value;
    }

    // Returns number of records removed 
    virtual int RemoveRecord (const string& key){
        return table_.erase(key);
    }

    // Returns a record
    virtual shared_ptr<ATTRIB_MAP> GetRecord(const string& key) {
        if (table_.find(key) != end(table_)) {
            return table_[key];
        }
        return nullptr;
    }

    // Queries each row of the table returns a match
    // If the col_list is "*" returns all rows
    virtual void Query(stringstream& ss,
        const string& col_list = "",
        const string& filter="") {
        TABLE::iterator row_iter =  table_.begin();
        for (;row_iter != end(table_); ++row_iter) {
          ROW r = *row_iter; 
          QueryEachUniqueKey(r, filter, col_list, ss);
        }
    }

    virtual const string GetUniqueKeyName(){
        return "";
    }

    map<string, shared_ptr<ATTRIB_MAP>>& GetInternalMap(){
        return table_;
    }

    void Schema(stringstream& ss) {
        for (auto& r: schema_attributes_){
            ss << r << endl;
        }
    }


private:
    //  tables share this method
    //  make it a static helper somewhere
    void QueryEachUniqueKey(std::pair<string, shared_ptr<ATTRIB_MAP>>& elem, 
        const string& filters, 
        const string& column_list, 
        stringstream& ss) {

        // Parse the column list
        vector<string> columns;
        if (!column_list.empty() &&
            column_list != "*"){
            CHAR_SEP comma_sep(",");
            boost::tokenizer<CHAR_SEP> elem_tok(column_list, comma_sep);
            boost::tokenizer<CHAR_SEP>::iterator iter = elem_tok.begin();
            boost::tokenizer<CHAR_SEP>::iterator end = elem_tok.end();
            for (;iter != end; ++iter){
                string col = *iter;
                boost::trim(col);
                columns.push_back(col);
            }
        }

        // Parse the filter spec
        bool can_print = true;
        vector<pair<string, string>> filter_vec;
        if (!filters.empty()){
            // Seperate the filters if there are multiple
            CHAR_SEP comma_sep(",");
            boost::tokenizer<CHAR_SEP> elem_tok(filters, comma_sep);
            boost::tokenizer<CHAR_SEP>::iterator iter = elem_tok.begin();
            boost::tokenizer<CHAR_SEP>::iterator end = elem_tok.end();
            for (;iter != end; ++iter){
                string filter = *iter;
                boost::trim(filter);

                // Deal with the individual filters
                CHAR_SEP eq_sep("=");
                boost::tokenizer<CHAR_SEP> elem_tok_filter(filter, eq_sep);
                boost::tokenizer<CHAR_SEP>::iterator iter_filter = elem_tok_filter.begin();
                boost::tokenizer<CHAR_SEP>::iterator end_filter = elem_tok_filter.end();
                if (std::distance(iter_filter,end_filter) >= 2){
                    string key, value;
                    key = *(iter_filter);
                    boost::trim(key);
                    value =  *(++iter_filter);
                    boost::trim(value);
                    filter_vec.push_back(make_pair(key,value));
                }
            }

            // The filter is applied...
            // Search for key in the attribute map 
            for(auto& kv : filter_vec) {
                string key = kv.first;
                string value = kv.second;
                if (!key.empty() && !value.empty()){
                    if ((*(elem.second)).find(key) != (*(elem.second)).end()){
                        if ((*(elem.second))[key] != value){
                            can_print = false; // filtered out
                        }
                    }
                }
            }
        }

        bool uk_printed = false;
        for (auto& e: *(elem.second)) {
            if (can_print && ((column_list =="*") || 
               (find(columns.begin(), columns.end(),e.first) != columns.end()))){
                if (!uk_printed){
                    ss << "\n\nUnique Key: " << elem.first << std::endl ;
                    uk_printed = true;
                }
                ss << e.first << " = " << e.second << std::endl;
            } else {
                continue;
            }
        }
    }


    //////////////////////////////////////////////////
    // Table data structure
    // [meta op id, attrib_map]
    //             [attrib_name, attrib_value].
    /////////////////////////////////////////////////
    map<string, shared_ptr<ATTRIB_MAP>> table_;

    // List of all the attributes stored in the table
    unordered_set<string> schema_attributes_;
protected:
    const string unique_key_;
};

// work_id : [File path, Metaop ID]
struct FileWorkIDMopTable : Table {
    FileWorkIDMopTable()
    {}

    ~FileWorkIDMopTable(){}

    virtual const string GetUniqueKeyName(){
        return "WORK_ID";
    }

};

// File path, vdisk ID, replicating node
struct FailFastFatalTable : Table {
    FailFastFatalTable()
    {}

    ~FailFastFatalTable(){}

    virtual const string GetUniqueKeyName(){
        return "OPID";
    }

};


// File path, vdisk ID, replicating node
struct FileVDiskTable : Table {
    FileVDiskTable()
    {}

    ~FileVDiskTable(){}

    virtual const string GetUniqueKeyName(){
        return "FILE_PATH";
    }

};

// File path, vdisk ID, replicating node
struct MasterIpTimestampTable : Table {
    MasterIpTimestampTable()
    {}

    ~MasterIpTimestampTable(){}

    virtual const string GetUniqueKeyName(){
        return "TIMESTAMP";
    }

};

// Attributes of all completed meta ops
struct CompletedMetaopsTable : Table {
    CompletedMetaopsTable()
    {}
    
    ~CompletedMetaopsTable(){}

    virtual const string GetUniqueKeyName(){
        return "Meta opid";
    }

};

// History of the meta op
struct MopsHistoryTable : Table {
    MopsHistoryTable()
    {}
    
    ~MopsHistoryTable(){}

    virtual const string GetUniqueKeyName(){
        return "meta_opid";
    }
};

// Join 
// Initialize with 2 objects ltable and rtable
struct EqJoin {
    EqJoin(shared_ptr<Table> lt, shared_ptr<Table> rt)
        :lt_(lt), rt_(rt)
    {}

    ~EqJoin()
    {}

    // Joins lt and rt based of lt_col_to_join
    // and the unique key col on rt
    // The results are put into ss
    void Exec(stringstream& ss,
        const string& lt_col_to_join,
        const string& col_list="",
        const string& filter=""){
        Table lt = *lt_;
        for(auto& l_row: lt_->GetInternalMap()){

            Table::ATTRIB_MAP attrib_map_lt = *l_row.second;
            if (attrib_map_lt.find(lt_col_to_join) == end(attrib_map_lt)){
                continue;
            }
            
            // Empty value in left table column
            const string& l_value = attrib_map_lt[lt_col_to_join];
            if (l_value == ""){
                continue;
            }

            // Matching value does NOT exist in right table
            if(rt_->GetRecord(l_value) == nullptr) {
                continue;
            }

            ss << "------------------------------------------------------" << endl;
            Table::ATTRIB_MAP attrib_map_rt = *(rt_->GetRecord(l_value));
            // Fill the ss with the result
            // "key : value"
            // Fill left table attribs
            // ss << lt_->GetTableName() << endl;
            ss << "LEFT TABLE" << endl;
            for (auto& l_att : attrib_map_lt){
                ss << l_att.first << " : " << l_att.second << endl;
            }

            // Fill right table attribs
            // ss << rt_->GetTableName() << endl;
            ss << "RIGHT TABLE" << endl;
            for (auto& r_att : attrib_map_rt){
                ss << r_att.first << " : " << r_att.second << endl;
            }

            ss << "------------------------------------------------------" << endl;
        }
    }
private:
    shared_ptr<Table> lt_;
    shared_ptr<Table> rt_;
};

// Map that stores a string of class name to the actual object
// This might help if the underlying Table datastructure
// is different for different tables
struct TablenameClassMap {
    typedef shared_ptr<TablenameClassMap> Ptr;
    typedef shared_ptr<const TablenameClassMap> PtrConst;

    TablenameClassMap(){
        // Pre-allocate space for map?
        // Threading model?
        // Policy based map?
    }
    ~ TablenameClassMap(){}
    
    shared_ptr<Table>  RegisterClass(const string& table_name,
        shared_ptr<Table> table_obj) {
        if (table_class_map_.find(table_name) ==
            end(table_class_map_)){
            table_class_map_[table_name] = table_obj;
        }
        return table_obj;
    }

    void DeRegisterClass(const string& table_name) {
        if (table_class_map_.find(table_name) !=
            end(table_class_map_)){
            table_class_map_.erase(table_name);
        }
    }

    shared_ptr<Table> GetTableObject(const string& table_name) {
        if (table_name.empty())
            return nullptr;

        if (table_class_map_.find(table_name) !=
            end(table_class_map_)){
            return table_class_map_[table_name];
        } else {
            return nullptr;
        }
    }

private:
    // Table is the Base class for all the tables
    // Internally each table stores 
    map<string,shared_ptr<Table>> table_class_map_;
};

// Maybe this should be variadic template
// with multiple forms of storage configured
template <typename Parser1,
          typename Parser2,
          typename Parser3,
          typename Parser4,
          typename Parser5,
          typename Parser6,
          typename Parser7>
struct CerebroInfoLog : public Parser1, Parser2, Parser3, Parser4, Parser5, Parser6, Parser7
{
  CerebroInfoLog()
     :table_map_(make_shared<TablenameClassMap>()){
      // Register table class in the class name map

  }

  virtual ~CerebroInfoLog(){}

  void ParseLine(const string& line){
      static_cast<Parser1*>(this)->ParseAndStore(line, table_map_);
      static_cast<Parser2*>(this)->ParseAndStore(line, table_map_);
      static_cast<Parser3*>(this)->ParseAndStore(line, table_map_);
      static_cast<Parser4*>(this)->ParseAndStore(line, table_map_);
      static_cast<Parser5*>(this)->ParseAndStore(line, table_map_);
      static_cast<Parser6*>(this)->ParseAndStore(line, table_map_);
      static_cast<Parser7*>(this)->ParseAndStore(line, table_map_);
  }

  void Query (stringstream& ss, const string& table, const string& col_list, const string& filter) {
      // Get the table object, execute query
      shared_ptr<Table> p_table = table_map_->GetTableObject(table);
      if (p_table != nullptr){
          p_table->Query(ss, col_list, filter);
      } else {
          ss << "No table by that name has been loaded\n";
      }
  }

  void Join (stringstream& ss,
     const string& l_table,
     const string& r_table,
     const string& col_to_join,
     const string& col_list,
     const string& filter) {
      EqJoin e_join(table_map_->GetTableObject(l_table),
          table_map_->GetTableObject(r_table));
      e_join.Exec(ss, col_to_join);

  }
private:
  // Maintain a class map, maps string table name
  // to Object
  shared_ptr<TablenameClassMap> table_map_;
};

} } } //namespace

#endif
