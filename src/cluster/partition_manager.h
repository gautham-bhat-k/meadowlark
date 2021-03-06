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

#ifndef PARTITION_MANAGER_H
#define PARTITION_MANAGER_H


#include <cstddef> // size_t
#include <map>
#include <vector>

#include "cluster/config.h"

namespace radixtree {

class PartitionManager {
public:
    PartitionManager(Config *config);
    ~PartitionManager();

    void Init();
    NodeID FindNode(PartitionID p);
    std::vector<NodeID> FindNodes(PartitionID p);

    std::vector<KVS> FindPartitionsByNode(NodeID n);

    void Print();
private:
    Config *config_;
    size_t partition_cnt_;
    size_t node_cnt_;
    Config::ReplicationScheme replication_scheme_;
    uint64_t replication_factor_;

    std::map<PartitionID, KVS> partition_;
    std::map<PartitionID, std::vector<NodeID>> partition2node_;
    std::map<NodeID, std::vector<PartitionID>> node2partition_;
};

} // namespace radixtree

#endif
