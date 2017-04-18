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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <boost/filesystem.hpp>

#include "radixtree/radixtree_libpmem.h"
#include "radixtree/radixtree_fam_atomic.h"
#include "radixtree/radix_tree.h"

#include "nvmm/epoch_manager.h"
#include "nvmm/memory_manager.h"
#include "nvmm/heap.h"

using namespace radixtree;
using namespace nvmm;

static const PoolId heap_id = 1; // assuming we only use heap id 1
static size_t heap_size = 128*1024*1024; // 128MB

struct MyNode {
    int64_t my_value;
};

void Init()
{
    EpochManager::Start();
    MemoryManager::Start();
}

GlobalPtr str2gptr(std::string root_str) {
    std::string delimiter = ":";
    size_t loc = root_str.find(delimiter);
    if (loc==std::string::npos)
        return 0;
    std::string shelf_id = root_str.substr(1, loc-1);
    std::string offset = root_str.substr(loc+1, root_str.size()-3-shelf_id.size());
    return GlobalPtr((unsigned char)std::stoul(shelf_id), std::stoul(offset));
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "demo_radix_tree: usage: demo_radix_tree root_ptr {create_tree,destroy_tree,get,put,destory,list} [<key> [<integer value>]]\n");
        exit(1);
    }
    std::string root_str(argv[1]);
    GlobalPtr root = str2gptr(std::string(root_str));

    std::string key;
    int64_t     my_value = 42;

    std::string command = argv[2];
    if (argc > 3)
        key = argv[3];
    if (argc > 4)
        my_value = atoi(argv[4]);

    RadixTree::key_type my_key;
    memset(&my_key, 0, sizeof(my_key));
    strcpy((char*)&my_key, key.c_str());

    // init memory manager and heap
    Init();
    ErrorCode ret;
    EpochManager *em = EpochManager::GetInstance();
    MemoryManager *mm = MemoryManager::GetInstance();
    Heap *heap = mm->FindHeap(heap_id);
    if (heap==NULL) {
        // create the radix tree if it does not exist
        ret = mm->CreateHeap(heap_id, heap_size);
        assert(ret == NO_ERROR);
        // init the heap
        heap = mm->FindHeap(heap_id);
    }
    assert(heap!=NULL);
    // open the heap
    ret = heap->Open();
    assert(ret == NO_ERROR);

    // init the radix tree
    RadixTree *tree = NULL;

    // handle 'create_tree' and 'destroy_tree'
    if (command == "create_tree") {
        tree = new RadixTree(mm, heap);
        assert(tree!=NULL);
        root = tree->get_root();
        // print out the root ptr
        std::cout << "Created a radix tree; its root pointer is " << root << std::endl;
        delete tree;
        heap->Close();
        delete heap;
        return 0;
    }
    else if (command == "destroy_tree") {
        EpochOp op(em);
        heap->Free(op, root);
        // print out the root ptr
        std::cout << "Destroyed a radix tree; its root pointer is " << root << std::endl;
        delete tree;
        heap->Close();
        delete heap;
        return 0;
    }

    // handle 'put', 'get', 'destroy', and 'list'
    if (root == 0) {
        std::cout << "Invalid root pointer " << root << std::endl;
        return -1;
    }

    assert(root!=0);
    // open the radix tree
    tree = new RadixTree(mm, heap, root);
    assert(tree!=NULL);
    // print out the root ptr
    std::cout << "Opened a radix tree; its root pointer is " << root << std::endl;

    char c = command[0];
    switch (c) {
    case 'g': {
        GlobalPtr result = tree->get(my_key, (int)key.size());
        if (result == 0)
            std::cout << "  not found: " << my_key << std::endl;
        else {
            MyNode* n = (MyNode*)mm->GlobalToLocal(result);
            pmem_invalidate(n, sizeof(n));
            std::cout <<"  " << my_key << " -> " << n->my_value << " " << result << std::endl;
        }
        break;
    }

    case 'h': {
        EpochOp op(em);
        GlobalPtr result = tree->get(my_key, (int)key.size());
        if (result == 0)
            std::cout << "  not found: " << my_key << std::endl;
        else {
            MyNode* n = (MyNode*)mm->GlobalToLocal(result);
            pmem_invalidate(n, sizeof(n));
            std::cout <<"  " << my_key << " -> " << n->my_value << " " << result << std::endl;
            while(getchar()!='x') {
                std::cout <<"  accessing " << result << ": " << n->my_value << std::endl;
            }
        }
        break;
    }

    case 'p': {
        EpochOp op(em);
        GlobalPtr my_node_ptr = heap->Alloc(op, sizeof(MyNode));
        std::cout << "  allocated memory at " << my_node_ptr << " for value " << my_value << std::endl;
        MyNode* n = (MyNode*)mm->GlobalToLocal(my_node_ptr);
        n->my_value = my_value;
        pmem_persist(n, sizeof(n));

        GlobalPtr result = tree->put(my_key, (int)key.size(), my_node_ptr);
        if (result==0)
            std::cout << "  successfully inserted " << my_key << " = " << my_value << " " << my_node_ptr << std::endl;
        else {
            std::cout << "  successfully updated " << my_key << " = " << my_value << " " << my_node_ptr << std::endl;
            heap->Free(op, result);
            std::cout << "  delayed free " << result << std::endl;
        }
        break;
    }

    case 'd': {
        GlobalPtr result = tree->destroy(my_key, (int)key.size());
        if (result == 0)
            std::cout << "  not found: " << my_key << std::endl;
        else {
            MyNode* n = (MyNode*)mm->GlobalToLocal(result);
            pmem_invalidate(n, sizeof(n));
            std::cout << "  " << my_key << " no longer = " << n->my_value << " " << result << std::endl;
            EpochOp op(em);
            heap->Free(op, result);
            std::cout << "  delayed free " << result << std::endl;
        }
        break;
    }

    case 'l': {
        tree->list([&mm](const RadixTree::key_type key, const int key_size, GlobalPtr p) {
                MyNode* n = (MyNode*)mm->GlobalToLocal(p);
                pmem_invalidate(n, sizeof(n));
                std::cout <<"  " << key << " -> " << n->my_value << " " << p << std::endl;
            });
        break;
    }

    case 'm': {
        while(1)
        {
            {
                // Begin epoch in a new scope block so that we exit the epoch when we out of scope
                // and don't block others when we then sleep.
                EpochOp op(em);
            }
            usleep(100);
        }
        break;
    }
    default:
        break;
    }

    delete tree;
    heap->Close();
    delete heap;

    return 0;
}
