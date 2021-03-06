/*
 *  (c) Copyright 2016-2017 Hewlett Packard Enterprise Development Company LP.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As an exception, the copyright holders of this Library grant you permission
 *  to (i) compile an Application with the Library, and (ii) distribute the
 *  Application containing code generated by the Library and added to the
 *  Application during this compilation process under terms of your choice,
 *  provided you also meet the terms and conditions of the Application license.
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <cstddef> // size_t
#include <map>
#include <iostream>
#include <assert.h>
#include <string>

namespace radixtree {

using PartitionID = uint64_t;
using NodeID = uint64_t;
using ServerID = uint64_t;

using Port = uint64_t;
using Addr = std::string;

using KVS = std::map<std::string, std::string>;

class Config {
public:
    enum class ReplicationScheme {
        INVALID, //
        NO_REPLICATION, //
        MASTER_SLAVE, //
        DYNAMO, //
        MODC //
    };

    Config()
        : shelf_base_(""), shelf_user_(""),
          partition_cnt_(0), server_cnt_(0), node_cnt_(0), kvs_type_(""), kvs_size_(0), cache_size_(0), 
          replication_scheme_(ReplicationScheme::NO_REPLICATION)
        {
        }

    ~Config() {}

    int LoadConfigFile(std::string path);
    void PrintConfigFile(std::string path);
    int SaveConfigFile(std::string path);

    bool IsValid();
    void Print();

    // NVMM
    void SetShelfBase(std::string shelf_base) {
        shelf_base_ = shelf_base;
    }
    void SetShelfUser(std::string shelf_user) {
        shelf_user_ = shelf_user;
    }

    std::string GetShelfBaseForPartition(PartitionID pid) {
        return shelf_base_+"/"+std::to_string(pid);
    }

    std::string GetShelfBase() {
        return shelf_base_;
    }

    std::string GetShelfUser() {
        return shelf_user_;
    }

    std::string GetShelfUserForPartition(PartitionID pid) {
        return shelf_user_+"_"+std::to_string(pid);
    }

    // KVS
    int UpdateRoot(PartitionID pid, std::string root);


    ReplicationScheme str2rs(std::string str);
    std::string rs2str(ReplicationScheme rs);

    void SetPartitionCnt(size_t partition_cnt) {
        partition_cnt_ = partition_cnt;
    }

    void SetServerCnt(size_t server_cnt) {
        server_cnt_ = server_cnt;
    }

    void SetNodeCnt(size_t node_cnt) {
        node_cnt_ = node_cnt;
    }

    void SetStartingPort(uint64_t starting_port) {
        starting_port_ = starting_port;
    }

    void SetKVSType(std::string type) {
        kvs_type_ = type;
    }

    void SetKVSSize(size_t size) {
        kvs_size_ = size;
    }

    void SetCacheSize(size_t size) {
        cache_size_ = size;
    }

    void SetServerThread(uint64_t cnt) {
        server_thread_ = cnt;
    }

    void SetReplicationScheme(ReplicationScheme replication_scheme) {
        replication_scheme_ = replication_scheme;
    }

    void SetReplicationFactor(uint64_t replication_factor=1) {
        switch (replication_scheme_) {
        case ReplicationScheme::NO_REPLICATION:
            replication_factor_ = 1;
            return;
        case ReplicationScheme::MASTER_SLAVE:
            assert(partition_cnt_>0 && node_cnt_>0 && node_cnt_>=partition_cnt_);
            assert(server_cnt_ >= replication_factor);
            replication_factor_ = node_cnt_/partition_cnt_;
            return;
        case ReplicationScheme::DYNAMO:
            assert(partition_cnt_>0 && node_cnt_>0 && replication_factor<=node_cnt_);
            assert(server_cnt_ >= replication_factor);
            //assert(partition_cnt_ == node_cnt_);
            replication_factor_ = replication_factor;
            return;
        case ReplicationScheme::MODC:
            assert(partition_cnt_>0 && node_cnt_>0 && replication_factor<=node_cnt_);
            assert(server_cnt_ >= replication_factor);
            //assert(partition_cnt_ == node_cnt_);
            replication_factor_ = replication_factor;
            return;
        case ReplicationScheme::INVALID:
        default:
            assert(0);
            return;
        }
    }

    size_t GetPartitionCnt() {
        return partition_cnt_;
    }

    size_t GetServerCnt() {
        return server_cnt_;
    }

    size_t GetNodeCnt() {
        return node_cnt_;
    }

    uint64_t GetStartingPort() {
        return starting_port_;
    }

    std::string GetKVSType() {
        return kvs_type_;
    }

    size_t GetKVSSize() {
        return kvs_size_;
    }

    size_t GetCacheSize() {
        return cache_size_;
    }

    uint64_t GetServerThread(uint64_t cnt) {
        return server_thread_;
    }

    std::map<PartitionID, std::map<std::string,std::string>> const &GetPartitions() {
        return partition_;
    }

    // std::map<NodeID, Port> const &GetNodes() {
    //     return node_;
    // }

    std::map<ServerID, Addr> const &GetServers() {
        return server_;
    }

    ReplicationScheme GetReplicationScheme() {
        return replication_scheme_;
    }

    uint64_t GetReplicationFactor() {
        return replication_factor_;
    }

    // TODO: read current state from FAM

private:

    // NVMM
    std::string shelf_base_;
    std::string shelf_user_;

    // KVS
    size_t partition_cnt_;
    size_t server_cnt_;

    size_t node_cnt_;
    uint64_t starting_port_;

    std::string kvs_type_;
    size_t kvs_size_;

    // memcached
    size_t cache_size_;
    uint64_t server_thread_;

    std::map<PartitionID, KVS> partition_; // partition id -> KVS
    //std::map<NodeID, Port> node_;
    std::map<ServerID, Addr> server_;

    ReplicationScheme replication_scheme_;
    uint64_t replication_factor_;

    int AddPartition(PartitionID pid, KVS kvs);
    int AddServer(ServerID sid, Addr ip);
    //int AddNode(NodeID nid, Port port);
    void RemovePartition(PartitionID pid);
    void RemoveServer(ServerID sid);
    //void RemoveNode(NodeID nid);
};

} // namespace radixtree

#endif
