// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <mutex>
#include <condition_variable>

#include "common/status.h"
#include "gen_cpp/Types_types.h"
#include "olap/olap_common.h"
#include "olap/storage_engine.h"
#include "olap/rowset/rowset_id_generator.h"

namespace doris {

// A DataDir used to manange data in same path.
// Now, After DataDir was created, it will never be deleted for easy implementation.
class DataDir {
public:
    DataDir(const std::string& path,
            int64_t capacity_bytes = -1,
            TabletManager* tablet_manager = nullptr,
            TxnManager* txn_manager = nullptr);
    ~DataDir();

    Status init();

    const std::string& path() const { return _path; }
    const size_t path_hash() const { return _path_hash; }
    bool is_used() const { return _is_used; }
    void set_is_used(bool is_used) { _is_used = is_used; }
    int32_t cluster_id() const { return _cluster_id; }
    DataDirInfo get_dir_info() {
        DataDirInfo info;
        info.path = _path;
        info.path_hash = _path_hash;
        info.is_used = _is_used;
        info.capacity = _capacity_bytes;
        return info;
    }

    // save a cluster_id file under data path to prevent
    // invalid be config for example two be use the same
    // data path
    Status set_cluster_id(int32_t cluster_id);
    void health_check();

    OLAPStatus get_shard(uint64_t* shard);


    OlapMeta* get_meta() { return _meta; }

    bool is_ssd_disk() const {
        return _storage_medium == TStorageMedium::SSD;
    }

    TStorageMedium::type storage_medium() const { return _storage_medium; }

    OLAPStatus register_tablet(Tablet* tablet);
    OLAPStatus deregister_tablet(Tablet* tablet);
    void clear_tablets(std::vector<TabletInfo>* tablet_infos);

    std::string get_absolute_tablet_path(TabletMeta* tablet_meta, bool with_schema_hash);

    std::string get_absolute_tablet_path(OLAPHeaderMessage& olap_header_msg, bool with_schema_hash);

    std::string get_absolute_tablet_path(TabletMetaPB* tablet_meta, bool with_schema_hash);

    std::string get_absolute_shard_path(const std::string& shard_string);

    void find_tablet_in_trash(int64_t tablet_id, std::vector<std::string>* paths);

    static std::string get_root_path_from_schema_hash_path_in_trash(const std::string& schema_hash_dir_in_trash);

    OLAPStatus next_id(RowsetId* id) {
        return _id_generator->get_next_id(id);
    }

    OLAPStatus set_next_id(RowsetId new_rowset_id) {
        return _id_generator->set_next_id(new_rowset_id);
    }

    // load data from meta and data files
    OLAPStatus load();

    void add_pending_ids(const std::string& id);

    void remove_pending_ids(const std::string& id);

    // this function scans the paths in data dir to collect the paths to check
    // this is a producer function. After scan, it will notify the perform_path_gc function to gc
    void perform_path_scan();

    // this function is a consumer function
    // this function will collect garbage paths scaned by last function
    void perform_path_gc();

private:
    std::string _cluster_id_path() const { return _path + CLUSTER_ID_PREFIX; }
    Status _init_cluster_id();
    Status _check_path_exist();
    Status _init_extension_and_capacity();
    Status _init_file_system();
    Status _init_meta();

    Status _check_disk();
    OLAPStatus _read_and_write_test_file();
    Status _read_cluster_id(const std::string& path, int32_t* cluster_id);
    Status _write_cluster_id_to_path(const std::string& path, int32_t cluster_id); 
    OLAPStatus _clean_unfinished_converting_data();
    OLAPStatus _convert_old_tablet();
    OLAPStatus _remove_old_meta_and_files(const std::set<int64_t>& tablet_ids);

    void _remove_check_paths_no_lock(const std::set<std::string>& paths);

    void _process_garbage_path(const std::string& path);

    void _remove_check_paths(const std::set<std::string>& paths);

    bool _check_pending_ids(const std::string& id);

private:
    std::string _path;
    size_t _path_hash;
    uint32_t _rand_seed;

    std::string _file_system;
    int64_t _capacity_bytes;
    TabletManager* _tablet_manager;
    TxnManager* _txn_manager;
    int32_t _cluster_id;
    int64_t _available_bytes;
    int64_t _used_bytes;
    uint64_t _current_shard;
    bool _is_used;
    // This flag will be set true if this store was not in root path when reloading
    bool _to_be_deleted;

    std::mutex _mutex;
    TStorageMedium::type _storage_medium;  // 存储介质类型：SSD|HDD
    std::set<TabletInfo> _tablet_set;

    static const size_t TEST_FILE_BUF_SIZE = 4096;
    static const size_t DIRECT_IO_ALIGNMENT = 512;
    static const uint32_t MAX_SHARD_NUM = 1024;
    char* _test_file_read_buf;
    char* _test_file_write_buf;
    OlapMeta* _meta = nullptr;
    RowsetIdGenerator* _id_generator = nullptr;

    std::set<std::string> _all_check_paths;
    std::mutex _check_path_mutex;
    std::condition_variable cv;

    std::set<std::string> _pending_path_ids;
    RWMutex _pending_path_mutex;
};

} // namespace doris
