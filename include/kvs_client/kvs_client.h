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

#ifndef KVS_CLIENT_H
#define KVS_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <cstddef> // size_t
#include <unordered_map>
#include <functional> // hash

#include "cluster/config.h"
#include "cluster/cluster.h"

struct memcached_st;


namespace radixtree {

struct LocationHash {
    size_t operator()(Location const &loc) const {
        return std::hash<std::string>()(loc.ip) ^ std::hash<uint64_t>()(loc.port);
    }
};

// TODO: the server map is NOT thread-safe
class KVSServer {
public:
    KVSServer();
    ~KVSServer();

    int Init (std::string config_file);
    void Final();

    int Put (char const *key, size_t const key_len,
             char const *val, size_t const val_len);
    int Get (char const *key, size_t const key_len,
             char *val, size_t &val_len);
    int Del (char const *key, size_t const key_len);

    void PrintCluster();
    void WipeServers();

    // TODO: get this from config file?
    size_t MaxKeyLen() {return 40;}

private:
    std::unordered_map<Location, memcached_st*, LocationHash> servers_;
    Config config_;
    Cluster cluster_;

    Location pick_a_server(char const *key, size_t const key_len);
    int server_init(Location loc);
    bool server_exist(Location loc);
    memcached_st **get_server(char const *key, size_t const key_len);
};

}
#endif // KVS_CLIENT_H
