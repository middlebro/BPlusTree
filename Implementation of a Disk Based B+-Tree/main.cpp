//
//  main.cpp
//  Implementation of a Disk Based B+-Tree
//
//  Created by 서형중 on 16/05/2019.
//  Copyright © 2019 Hyeongjung Seo. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <queue>

#define physical_offset_of_PageID(BID, blockSize) (12 + (BID - 1) * blockSize)
#define number_of_entries_per_node(blockSize) (blockSize - 4)/ 8
#define number_of_data_per_node(blockSize) blockSize / 4
using namespace std;

class BTree;
class Node;
class bp_entry {
public:
    bp_entry() {
        key = value = BID = 0;
    }
    void init() {
        key = value = BID = 0;
    }
private:
    int key;        // key for this entry
    int parent_BID;// Node had this entry
    int BID;     // Node pointed this entry
    int value;     // if this entry is in leaf node, pointing data
    
    friend class Node;
    friend class BTree;
};

class Node {
public:
    Node(int blockSize);
    
    bool fill_entry(int key, int value);
    bool insertEntry(int key, int BID);
    bool insertIndexEntry(int key, int BID);
    bool insertDataEntry(int key, int value);
    
    void copy_Node(Node* origin);
    bool is_leaf();
    bool IS_FULL();
private:
    // leaf node
    int NextBID;
    // non-leaf node
    int NextlevelBID;
    // common
    bp_entry *entries;
    int blockSize, cnt;
    
    Node *parent;
    int parent_BID, myBID;
    bool leaf;
    friend class BTree;
};

class BTree {
public:
    BTree(const char *filename);
    bool insert(int key, int rid);
    void print();
    int* search(int key);   // point search
    int* search(int startRange, int endRange);  // range search
    
    void set_o_filename(const char*);
private:
    // utility func.
    Node* split(Node *node, int key);
    Node* _split(Node *node, int key);
    
    bool find_leaf_node_by_key(int key, Node **node) ;
    int find_entry_by_key(Node *node, int key);
    
    // 입력받은 BID에 해당하는 Block의 data를 indexFile에서 read해서 해당 block을 반환.
    Node* find_Node_by_BID(int BID) ;
    Node* find_leaf_node_by_BID(int BID);
    
    
    bool IS_NODE_FULL(Node *v);
    bool insert_key(Node *target, int key, int value);
    void path_init();
    void bp_copy_node (Node *target_node, Node *node, bool start, int size) ;
    bool update_parent_entry(Node *parent, Node* child);
    
    // shifting entries to left and return NextlevelBID before shifting
    int bp_node_entry_left_shit(Node *v);
    // update_indexFile
    bool update_file_header_on_indexFile();
    bool update_root_on_indexFile();
    bool update_lnode_on_indexFile(Node* lnode);
    bool update_rnode_on_indexFile(Node* rnode);
    bool update_non_leaf_node_on_indexFile(Node *v);
    bool update_leaf_node_on_indexFile(Node* v);
    bool update_BID_on_indexFile(int lnode_BID);
    
    int find_last_leaf_node_BID ();
    
    
    fstream indexFile;
    const char *filename;
    const char *output_filename;
    int blockSize;  // the physical size of a B_plus-tree node.
    int rootBID;    // the root node's BID in B_plus-tree.bin file.
    int depth;      // check whether a node is leaf or not.
    Node* root;
    vector<Node*> path_root_to_leaf;
    
    friend class Node;
};

// TEST
int main(int argc, const char * argv[]) {
    char command = argv[1][0];
    // if inserted filename (btree.bin) is exist
        // open btree.bin and read block size from file
        // create Btree obj using inserted filename and blcok size value in btree.bin file
    // else
        // create file (inserted file name ;btree.bin)
        // create Btree obj using inserted filename and block size
    switch (command) {
        case 'c': {
            // create index file
            ofstream indexFile(argv[2], ios::trunc | ios::binary);
            if (indexFile.is_open()) {
                // initialize fileheader
                int blockSize = atoi(argv[3]);
                int rootBID = 1;
                int depth = 0;
                indexFile.write(reinterpret_cast<char*>(&blockSize), sizeof(int));
                indexFile.write(reinterpret_cast<char*>(&rootBID), sizeof(int));
                indexFile.write(reinterpret_cast<char*>(&depth), sizeof(int));
                // initialize root Block
                for (int i = 0; i < number_of_data_per_node(blockSize); ++i) {
                    int tmp = 0;
                    indexFile.write(reinterpret_cast<char*>(&tmp), sizeof(int));
                }
            }
            cout << argv[2] <<" file is sucessly created!\n";
            indexFile.close();
            break;
        }
        case 'i': {
            BTree *myBtree = new BTree(argv[2]);
            // insert recoeds from [records data file], ex) records.txt
            string line;
            ifstream recordFile(argv[3]);
            if (recordFile.is_open()) {
                while (getline(recordFile, line)) {
                    stringstream line_of_recordFile(line);
                    string key, id;
                    getline(line_of_recordFile, key, ',');
                    getline(line_of_recordFile, id);
                    myBtree->insert(atoi(key.c_str()), atoi(id.c_str()));
                }
            }
            else
                cout << "Cannot find File : " << argv[3] << " in this directory. \n";
            recordFile.close();
            break;
        }
        case 's': {
            BTree *myBtree = new BTree(argv[2]);
            // Point(exact) search
            // search keys in [input file] and print results to [output file]
            string key;
            ifstream input(argv[3]);
            ofstream output(argv[4], ios::trunc);
            if (input.is_open()) {
                while (getline(input, key)) {
                    int *pair = myBtree->search(atoi(key.c_str()));
                    output << pair[0] <<  ", " << pair[1] << "\n";
                }
            }
            else
                cout << "Cannot find File : " << argv[3] << " in this directory. \n";
            input.close();
            output.close();
            break;
        }
        case 'r': {
            BTree *myBtree = new BTree(argv[2]);
            // Range search
            // search keys in [input file] and print results to [output file]
            myBtree->set_o_filename(argv[4]);
            string line;
            ifstream input(argv[3]);
            ofstream output(argv[4], ios::trunc);
            if (input.is_open()) {
                while (getline(input, line)) {
                    stringstream line_of_input(line);
                    string startRange, endRange;
                    getline(line_of_input, startRange, ',');
                    getline(line_of_input, endRange, ',');
                    int *result = myBtree->search(atoi(startRange.c_str()), atoi(endRange.c_str()));
                    for (int i = 0; i < sizeof(result)/sizeof(int); ++i) {
                        output << result[i] << ", " << result[i + 1] << "\t";
                        ++i;
                    }
                    output << "\n";
                }
            }
            else
                cout << "Cannot find File : " << argv[3] << " in this directory. \n";
            input.close();
            output.close();
            break;
        }
        case 'p': {
            // print (B+)-Tree structure to [output file]
            BTree *myBtree = new BTree(argv[2]);
            myBtree->set_o_filename(argv[3]);
            myBtree->print();
            break;
        }

        default:
            cout << "First option must be one of \"c, i, s, r, p\"\n";
    }
    return 0;
}

// Node::definition
Node::Node(int blockSize) {
    NextBID = NextlevelBID = cnt = parent_BID = myBID = 0;
    this->blockSize = blockSize;
    this->parent = NULL;
    entries = new bp_entry[number_of_entries_per_node(blockSize)];
}
bool Node::fill_entry(int key, int value) {
    if (is_leaf()) {
        entries[cnt].key = key;
        entries[cnt].value = value;
        ++cnt;
    }
    else {
        entries[cnt].key = key;
        entries[cnt].BID = value;
        ++cnt;
    }
    return true;
}
bool Node::insertEntry(int key, int BID) {
    ++cnt;
    if (is_leaf()) {
        return insertDataEntry(key, BID);
    }
    else
        return insertIndexEntry(key, BID);
}
bool Node::insertIndexEntry(int key, int BID) {
    int tmp = this->NextlevelBID;
    for (int i = 0; i <= this->cnt; ++i) {
        if (tmp < BID) {
            // 해당 엔트리 right shift
            for (int j = cnt; i < j; --j) {
                swap(this->entries[j], this->entries[j - 1]);
                ++entries[j].BID;
            }
            // insert <key, BID>
            this->entries[i].key = key;
            this->entries[i].BID = BID;
            ++this->cnt;
            break;
        }
        else
            tmp = this->entries[i].BID;
    }
    return true;
}
bool Node::insertDataEntry(int key, int value) {
    bool ret = this->fill_entry(key, value);
    assert(ret != false);
    for (int i = cnt - 1; i > 0; --i) {
        if (entries[i - 1].key > entries[i].key ) {
            swap(entries[i], entries[i - 1]);
        }
    }
    return true;
}
void Node::copy_Node(Node* origin) {
    this->NextBID = origin->NextBID;
    this->NextlevelBID = origin->NextlevelBID;
    this->entries = origin->entries;
    this->blockSize = origin->blockSize;
    this->cnt = origin->cnt;
    this->parent = origin->parent;
    this->parent_BID = origin->parent_BID;
    this->myBID = origin->myBID;
    this->leaf = origin->leaf;
}
bool Node::is_leaf() {
    return leaf;
}
bool Node::IS_FULL() {
    return (cnt == number_of_entries_per_node(blockSize)) ? true : false;
    }

// BTree::Definitions
BTree::BTree(const char *filename) {
    this->filename = filename;
    // B_plus tree data init.
    ifstream indexFile(filename, ios::in | ios::binary);
    if (indexFile.is_open()) {
        indexFile.seekg(0, ios::beg);
        indexFile.read(reinterpret_cast<char*>(&this->blockSize), sizeof(int));
        indexFile.read(reinterpret_cast<char*>(&this->rootBID), sizeof(int));
        indexFile.read(reinterpret_cast<char*>(&this->depth), sizeof(int));
    
        // root data init.
        this->root = new Node(blockSize);
        indexFile.seekg(physical_offset_of_PageID(rootBID, blockSize), ios::beg);
        // if root == non-leaf node
        if (depth > 0) {            indexFile.read(reinterpret_cast<char*>(&root->NextlevelBID), sizeof(int));
            for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
                int key, BID;
                indexFile.read(reinterpret_cast<char*>(&key), sizeof(int));
                indexFile.read(reinterpret_cast<char*>(&BID), sizeof(int));
                if (key != 0)
                    root->fill_entry(key, BID);
            }
        }
        else { // if root == leaf node
            for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
                int key, value;
                indexFile.read(reinterpret_cast<char*>(&key), sizeof(int));
                indexFile.read(reinterpret_cast<char*>(&value), sizeof(int));
                if (key != 0)
                    root->fill_entry(key, value);
            }
            indexFile.read(reinterpret_cast<char*>(&root->NextBID), sizeof(int));
        }
    }
    else
        cout << "Error : index file is not successly executed.\n";
   
    
    // set root node's status
    root->myBID = rootBID;
    if (depth == 0)
        root->leaf = true;
    else
        root->leaf = false;
    // end set root status

    indexFile.close();
}

bool BTree::insert(int key, int rid) {
    cout << "<<<<<<<Insertion>>>>>>>   key : " << key << endl;
    /* Insertion
     1. Find the leaf node in which the search-key value would appear
     2. If the search-key value is already present in the leaf node
     2.1. Add record to the file
     2.2. If necessary add a pointer to the bucket
     3. If the search-key value is not present, then
     3.1 add the record to the main file (and create a bucket if necessary)
     3.2 If there is room in the leaf node, insert<key-value, pointer>pair
     in the leaf node
     3.3 Otherwise, split the node (along with the new<key-value, pointer>entry)
     */
    /*
     1. find LEAF NODE
     2. is node FULL?
     2. 1. YES
     2. 1. 1 Split -> return to target LEAF NODE
     2. 2. NO
     2. 3. NOPE
     3. INSERT key to TARGET LEAF NODE
     */
    path_init();
    bool IS_SPLIT = false;
    Node *leafNode;
    bool ret = find_leaf_node_by_key(key, &leafNode); //
//    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
//        cout << (leafNode)->entries[i].key << " " << (leafNode)->entries[i].value << endl;
//    }
//    cout << leafNode->NextBID << endl;
    assert(ret != false);

    if (IS_NODE_FULL(leafNode)) {
        // Splitting a leaf node
        leafNode = split(leafNode, key);
        IS_SPLIT = true;
    }
    //overwrite record on leafNode in indexFile
    
//    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
//        cout << leafNode->entries[i].key << " " << leafNode->entries[i].value << " " << leafNode->entries[i].BID << " /";
//    }
//    cout << endl;
//
    ret = leafNode->insertDataEntry(key, rid);
    assert(ret != false);
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        cout << leafNode->entries[i].key << " " << leafNode->entries[i].value << " " << leafNode->entries[i].BID << " /";
    }
    cout << endl;
    // update parent's entry to first key of entry of leafNode
    if (IS_SPLIT) {
        update_parent_entry(leafNode->parent, leafNode);
        update_non_leaf_node_on_indexFile(leafNode->parent);
        cout << "leafNode->parent->myBID : " << leafNode->parent->myBID << endl;
        cout << leafNode->parent->NextlevelBID << " ";
        for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
            cout << leafNode->parent->entries[i].key << " " <<  leafNode->parent->entries[i].BID << " /";
        }
        cout << endl;
        cout << leafNode->myBID << endl;
        cout << leafNode->parent->myBID << endl;
    }
    
    return update_leaf_node_on_indexFile(leafNode);
}
void BTree::print() {
    ofstream output(this->output_filename, ios::trunc);
    queue<int> q;
    if (this->depth > 0) {
        output << "<0>\n";
        q.push(root->NextlevelBID);
        for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
            if (root->entries[i].BID != 0) {
                q.push(root->entries[i].BID);
                output << root->entries[i].key << ", ";
            }
            else break;        }
    }
    else {
        output << "<0>\n";
        for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
            if (root->entries[i].key != 0) {
                output << root->entries[i].value << ", ";
            }
            else {
                output.seekp(-2);
                output << " \n";
                break;
            }
        }
    }
    long depth(0), breath(0);
    while (!q.empty()) {
        if (breath == 0) {
            breath = q.size();
            output.seekp(-2, ios::end);
            output << " \n<" << ++depth << ">\n";
        }
        int BID = q.front(); q.pop();
        --breath;
        
        if (depth < this->depth) {
            Node *v = find_Node_by_BID(BID);
            q.push(v->NextlevelBID);
            for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
                if (v->entries[i].BID != 0) {
                    q.push(v->entries[i].BID);
                    output << v->entries[i].key << ", ";
                }
            }
        }
        else {
            Node *v = find_leaf_node_by_BID(BID);
            for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
                if (v->entries[i].value != 0) {
                    output << v->entries[i].key << ", ";
                }
            }
            output.seekp(-2, ios::end);
            output << " / ";
        }
    }
    output.seekp(-2, ios::end);
    output << " \n";
    output.close();
    return;
}
int* BTree::search(int key) { // point search
    Node *target_Node;
    bool ret = find_leaf_node_by_key(key, &target_Node);
    assert(ret != false);
    
    int *pair = new int[2];
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        if (target_Node->entries[i].key == key) {
            pair[0] = target_Node->entries[i].key;
            pair[1] = target_Node->entries[i].value;
            return pair;
        }
    }
    pair[0] = key;
    return pair; // Error : Search key is not exist in B PLUS TREE
}

int* BTree::search(int startRange, int endRange) { // range search
    Node *start_Node;
    bool ret = find_leaf_node_by_key(startRange, &start_Node);
    assert(ret != false);
    vector<int> result_pair;
    
    // range search
    bool outOfRange = false;
    while (outOfRange != true) {
        for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
            if (start_Node->entries[i].key >= startRange && start_Node->entries[i].key <= endRange) {
                result_pair.push_back(start_Node->entries[i].key);
                result_pair.push_back(start_Node->entries[i].value);
            }
            if (start_Node->entries[i].key > endRange) {
                outOfRange = true;
                break;
            }
        }
        start_Node = find_leaf_node_by_BID(start_Node->myBID + 1);
        if (start_Node->NextBID == 0)
            break;
    }
    
    // init return value;
    int *result = new int[result_pair.size()];
    for (int i = 0; i < result_pair.size(); ++i) {
        result[i] = result_pair[i];
    }
    return result;
}
Node* BTree::split(Node *node, int key) {
    /* split 연산
     1. 쪼개는 갯수는 floor ((Fanout + 1) / 2)에 해당함. (변수 split_factor)
     2. 쪼개는 기준값은 entries[split_factor + 1].key에 해당함
     3. 좌, 우 노드의 생성 및 노드를 반으로 쪼개 좌, 우로 입력함
     4. 해당 노드가 루트였나?
     4.1. Yes
     4.1.1. 새 노드 할당(Non-leaf로 할당한다.)
     4.1.2. 새 노드로 루트로 변경함.
     4.1.3. 좌, 우 노드의 부모를 해당 노드로 변경해준다.
     4.2. No
     4.2.1. 해당 노드도 꽉찼나?
     4.2.1.1. Yes
     4.2.1.1.1. 역시 쪼개기 연산 수행
     4.2.1.2. No
     5. 아무튼 부모 노드에 split key를 넣고, 해당 엔트리의 좌우노드 포인터를 좌, 우 노드로 바꿔준다.
     */
    
    int split_factor = (number_of_entries_per_node(blockSize) + 1) >> 1;
    int split_key = node->entries[split_factor].key;
    bool NODE_IS_ROOT = false;
    cout << "split key : " << split_key << endl;
    // 해당 노드의 parent 노드를 찾음
    Node *parent = node->parent;
    Node *lnode, *rnode;
    
    lnode = new Node(blockSize);
    rnode = new Node(blockSize);
    lnode->leaf = true;
    rnode->leaf = true;
    // lnode의 BID는 원래 노드의 BID를 그대로 사용
    // rnode의 BID는 lnode의 BID + 1
    // -> 새로운 노드가 추가 되었으므로 rnode 이후 모든 Node의 BID가 바뀜.
    enum {LEFT = true, RIGHT = false};
    if (number_of_entries_per_node(blockSize) % 2 != 0) {
        bp_copy_node(lnode, node, LEFT, split_factor + 1);
        bp_copy_node(rnode, node, RIGHT, split_factor);
    }
    else {
        bp_copy_node(lnode, node, LEFT, split_factor);
        bp_copy_node(rnode, node, RIGHT, split_factor);
    }
    // lnode, rnode NextBID init
    lnode->NextBID = node->NextBID;
    if (lnode->NextBID != 0)
        rnode->NextBID = lnode->NextBID + 1;
    else
        rnode->NextBID = 0;
    lnode->NextBID = rnode->myBID;
    // indexFile -> lnode 위치 + 1 부터 write rnode data (BlockSize 만큼)
    if (node == root) { // 첫번째 Split이 발생한다면 Node는 분명 Root임이 틀림없죠!ㅋ
        node = new Node(blockSize); // 새로운 root노드 생성
        node->NextlevelBID = lnode->myBID;
        node->myBID = root->myBID + 1;
        node->leaf = false;
        
        // have to set new Node's BID
        root = node;
        ++depth;
        // set lnode, rnode parent
        lnode->parent = node;
        rnode->parent = node;
        lnode->parent_BID = node->myBID;
        rnode->parent_BID = node->myBID;
        parent =  node;
        NODE_IS_ROOT = true;
    }
    else {
        if (IS_NODE_FULL(parent)) {
            parent = _split(parent, split_key);
        }
        delete node;
    }
    cout << "!L!";
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        cout << lnode->entries[i].key << " " << lnode->entries[i].value << " " << lnode->entries[i].BID << " /";
    }
    cout << lnode->NextBID << " ";
    cout << "!R!";
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        cout << rnode->entries[i].key << " " << rnode->entries[i].value << " " << rnode->entries[i].BID << " /";
    }
    cout << rnode->NextBID << " ";
    cout << endl;
    // parent 노드에 rnode 추가 <key, BID>

    parent->insertIndexEntry(rnode->entries[0].key, rnode->myBID);
    
    if (parent != root) {
        // apply to bin file
        // 1. update BID for bigger than lnode BID except root
        // 2. lnode update
        // 3. add rnode to bin file
        // 4. update
        
        // update non-leaf node's BID for non-leaf node's NextlevelBID, entry's BID > lnode BID, ++BID
        // (rnode path 오른쪽 Block 들에 대해 ++BID)
        update_BID_on_indexFile(lnode->myBID);
    }
    rootBID = ++root->myBID;
    // update lnode on indexFile
    update_lnode_on_indexFile(lnode);
    // update rnode on indexFile
    update_rnode_on_indexFile(rnode);
    // update root on indexFile
    fstream indexFile(filename, ios::in| ios::binary);
    indexFile.seekg(0, ios::end);
    long size = indexFile.tellg() / 4;
    // copy indexFile to tmp
    indexFile.seekg(0, ios::beg);
    cout << "START Index_File contents : ";
    for (int i = 0; i < size; ++i) {
        int x;
        indexFile.read(reinterpret_cast<char*>(&x), sizeof(int));
        cout << x << " ";
    }
    cout << endl;
    indexFile.close();
    if (NODE_IS_ROOT) {
        rootBID = root->myBID;
        update_root_on_indexFile();
    }
    // update file_header
    indexFile.open(filename, ios::in | ios::binary);
    
    indexFile.seekg(0, ios::end);
    size = indexFile.tellg() / 4;
    // copy indexFile to tmp
    indexFile.seekg(0, ios::beg);
    cout << "END Index_File contents : ";
    for (int i = 0; i < size; ++i) {
        int x;
        indexFile.read(reinterpret_cast<char*>(&x), sizeof(int));
        cout << x << " ";
    }
    cout << endl;

    update_file_header_on_indexFile();
    indexFile.open(filename, ios::in | ios::binary);
    
    indexFile.seekg(0, ios::end);
    size = indexFile.tellg() / 4;
    // copy indexFile to tmp
    indexFile.seekg(0, ios::beg);
    cout << "END Index_File contents : ";
    for (int i = 0; i < size; ++i) {
        int x;
        indexFile.read(reinterpret_cast<char*>(&x), sizeof(int));
        cout << x << " ";
    }
    cout << endl;

    return (key < split_key ? lnode: rnode);
}
Node* BTree::_split(Node *node, int key) {
    // have to modify list
    // 1. split_factor
    // 2. split 발생시 NextlevelBID 값 처리
    // 3. non-leaf node status 조정
    int split_factor = (number_of_entries_per_node(blockSize) + 1) >> 1;
    int split_key = node->entries[split_factor].key;
    bool NODE_IS_ROOT = false;
    cout << "split key : " << split_key << endl;
    // 해당 노드의 parent 노드를 찾음
    Node *parent = node->parent;
    Node *lnode, *rnode;
    lnode = new Node(blockSize);
    rnode = new Node(blockSize);
    lnode->leaf = false;
    rnode->leaf = false;
    // lnode의 BID는 원래 노드의 BID를 그대로 사용
    // rnode의 BID는 lnode의 BID + 1
    // -> 새로운 노드가 추가 되었으므로 rnode 이후 모든 Node의 BID가 바뀜.
    enum {LEFT = true, RIGHT = false};
    if (number_of_entries_per_node(blockSize) % 2 != 0) {
        bp_copy_node(lnode, node, LEFT, split_factor + 1);
        bp_copy_node(rnode, node, RIGHT, split_factor);
    }
    else {
        bp_copy_node(lnode, node, LEFT, split_factor);
        bp_copy_node(rnode, node, RIGHT, split_factor);
    }
    // lnode, rnode NextlevelBID init
    lnode->NextlevelBID = node->NextlevelBID;
    bp_node_entry_left_shit(rnode);
    
    // indexFile -> lnode 위치 + 1 부터 write rnode data (BlockSize 만큼)
    if (node == root) { // 첫번째 Split이 발생한다면 Node는 분명 Root임이 틀림없죠!ㅋ
        node = new Node(blockSize); // 새로운 root노드 생성
        node->NextlevelBID = lnode->myBID;
        node->myBID = root->myBID + 1;
        node->leaf = false;
        NODE_IS_ROOT = true;
        // have to set new Node's BID
        root = node;
        ++depth;
        // set lnode, rnode parent
        lnode->parent = node;
        rnode->parent = node;
        lnode->parent_BID = node->myBID;
        rnode->parent_BID = node->myBID;
        parent = node;
    }
    else {
        if (IS_NODE_FULL(parent)) {
            parent = _split(parent, split_key);
        }
        delete node;
    }
    // parent 노드에 rnode 추가 <key, BID>
    parent->insertIndexEntry(rnode->entries[0].key, rnode->myBID);
    
    if (parent != root) {
        // apply to bin file
        // 1. update BID for bigger than lnode BID except root
        // 2. lnode update
        // 3. add rnode to bin file
        // 4. update
        
        // update non-leaf node's BID for non-leaf node's NextlevelBID, entry's BID > lnode BID, ++BID
        // (rnode path 오른쪽 Block 들에 대해 ++BID)
        

    }
    update_BID_on_indexFile(lnode->myBID);
    rootBID = ++root->myBID;
    // update lnode on indexFile
    update_lnode_on_indexFile(lnode);
    // update rnode on indexFile
    update_rnode_on_indexFile(rnode);
    // update root on indexFile
    if (NODE_IS_ROOT) {
        rootBID = root->myBID;
        update_root_on_indexFile();
    }
    // update file_header
    update_file_header_on_indexFile();
    return (split_key < key ? lnode: rnode);
}
bool BTree::find_leaf_node_by_key(int key, Node **node) {
    // for i : 0 to depth -> depth 만큼 NextlevelNode로 접근해서 해당노드의 BID를
    Node *target = root;
    // find last non-leaf node
    for (int i = 0; i < this->depth - 1; ++i) {
        int nextBID = find_entry_by_key(target, key);
        target = find_Node_by_BID(nextBID);
        target->parent = path_root_to_leaf.back();
        path_root_to_leaf.push_back(target);
    }
    // find leaf node
    if (this->depth > 0) {
        int nextBID = find_entry_by_key(target, key);
        cout << nextBID <<"!"<< endl;
        target = find_leaf_node_by_BID(nextBID);
        target->parent = path_root_to_leaf.back();
        path_root_to_leaf.push_back(target);
    }
    if (root->is_leaf()) {
        cout << "Root\n";
        for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
            cout << root->entries[i].key << " " << root->entries[i].value << " ";
        }
        cout << root->NextBID << endl;
    }
    else {
        cout << "Root\n";
        cout << root->NextlevelBID << " ";
        for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
            cout << root->entries[i].key << " " << root->entries[i].BID << " ";
        }
        cout << endl;
    }
    
    cout << "Target\n";
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        cout << target->entries[i].key << " " << target->entries[i].value << " ";
    }
    cout << target->NextBID << endl;
    *node = target;
    return true;
}

int BTree::find_entry_by_key(Node *node, int key) {
    // 리프노드가 아닌 경우 해당 키 범위에 해당 되는 다음 레벨 노드의 BID를 반환.
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        if (node->entries[i].key != 0 && node->entries[i].key > key) {
            if (i != 0) {
                return node->entries[i - 1].BID;
            }
            return node->NextlevelBID;
        }
        else if (node->entries[i].key == 0)
            return node->entries[i - 1].BID;
    }
    return 0;    // return NULL value. ( 0 == NULL on BID )
}
Node* BTree::find_Node_by_BID(int BID) {
    ifstream indexFile(filename, ios::in | ios::binary);
    indexFile.seekg(physical_offset_of_PageID(BID, blockSize), ios::beg);
    Node *target = new Node(blockSize);
    target->leaf = false;
    indexFile.read(reinterpret_cast<char*>(&target->NextlevelBID), sizeof(int));
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        int key, BID;
        indexFile.read(reinterpret_cast<char*>(&key), sizeof(int));
        indexFile.read(reinterpret_cast<char*>(&BID), sizeof(int));
        if (key != 0)
            target->fill_entry(key, BID);
    }
    target->myBID = BID;
    
    indexFile.close();
    return target;
}
Node* BTree::find_leaf_node_by_BID(int BID) {
    ifstream indexFile(filename, ios::in | ios::binary);
    indexFile.seekg(physical_offset_of_PageID(BID, blockSize), ios::beg);
    Node *target = new Node(blockSize);
    target->leaf = true;
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        int key, value;
        indexFile.read(reinterpret_cast<char*>(&key), sizeof(int));
        indexFile.read(reinterpret_cast<char*>(&value), sizeof(int));
        if (key != 0)
            target->fill_entry(key, value);
    }
    indexFile.read(reinterpret_cast<char*>(&target->NextBID), sizeof(int));
    target->myBID = BID;
    
    indexFile.close();
    return target;
}
bool BTree::IS_NODE_FULL(Node *v) {
    if (v->IS_FULL())
        return true;
    else
        return false;
}

bool BTree::insert_key(Node *target, int key, int value) {
    return target->insertDataEntry(key, value);
}
void BTree::path_init() {
    path_root_to_leaf.erase(path_root_to_leaf.begin(),path_root_to_leaf.end());
    path_root_to_leaf.push_back(root);
}
void BTree::bp_copy_node (Node *target_node, Node *node, bool start, int size) {
    enum {LEFT = true, RIGHT = false};
    if (start == LEFT) {
        target_node->myBID = node->myBID;
        target_node->parent_BID = node->parent_BID;
        target_node->parent = node->parent;
        for (int i = 0; i < size; ++i) {
            target_node->fill_entry(node->entries[i].key, node->entries[i].value);
        }
    }
    else {
        target_node->myBID = node->myBID + 1;
        target_node->parent_BID = node->parent_BID;
        target_node->parent = node->parent;
        for (int i = size; i < number_of_entries_per_node(blockSize); ++i) {
            target_node->fill_entry(node->entries[i].key, node->entries[i].value);
        }
    }
}
void BTree::set_o_filename(const char* filename) {
    this->output_filename = filename;
}
int BTree::bp_node_entry_left_shit(Node *v) {
    int tmp_nextlevelBID = v->NextlevelBID;
    v->NextlevelBID = v->entries[0].BID;
    for (int i = 1; i < number_of_entries_per_node(blockSize) - 1; ++i) {
        v->entries[i - 1] = v->entries[i];
    }
    return tmp_nextlevelBID;
}
// BTree::Definition update indexFile
bool BTree::update_file_header_on_indexFile(){
    fstream indexFile(filename, ios::in | ios::out | ios::binary);
    indexFile.seekp(sizeof(int), ios::beg);
    indexFile.write(reinterpret_cast<char*>(&this->rootBID), sizeof(int));
    indexFile.write(reinterpret_cast<char*>(&this->depth), sizeof(int));
    indexFile.close();
    return true;
}
bool BTree::update_root_on_indexFile() {
    cout << "BTree::update_root_on_indexFile() : " << root->myBID << endl;
    fstream indexFile;
    indexFile.open(filename, ios::in | ios::app | ios::binary);
    indexFile.seekp(root->myBID, ios::end);
    indexFile.write(reinterpret_cast<char*>(&root->NextlevelBID), sizeof(int));
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        indexFile.write(reinterpret_cast<char*>(&root->entries[i].key), sizeof(int));
        indexFile.write(reinterpret_cast<char*>(&root->entries[i].value), sizeof(int));
    }
    indexFile.close();
    return true;
}

bool BTree::update_lnode_on_indexFile(Node* lnode) {
    cout << "BTree::update_lnode_on_indexFile(Node* lnode) : " << lnode->myBID << endl;
    fstream indexFile(filename, ios::in | ios::out | ios::binary);
    indexFile.seekp(physical_offset_of_PageID(lnode->myBID, blockSize), ios::beg);
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        indexFile.write(reinterpret_cast<char*>(&lnode->entries[i].key), sizeof(int));
        indexFile.write(reinterpret_cast<char*>(&lnode->entries[i].value), sizeof(int));
    }
    indexFile.write(reinterpret_cast<char*>(&lnode->NextBID), sizeof(int));
    indexFile.close();
    return true;
}
bool BTree::update_rnode_on_indexFile(Node* rnode) {
    cout << "BTree::update_rnode_on_indexFile(Node* rnode) : " << rnode->myBID << endl;
    fstream indexFile(filename, ios::in| ios::binary);
    fstream tmp("tmp", ios::out | ios::binary);
    indexFile.seekg(0, ios::end);
    long size = indexFile.tellg() / 4;
    // copy indexFile to tmp
    indexFile.seekg(0, ios::beg);
    cout << "Index_File contents : ";
    for (int i = 0; i < size; ++i) {
        int x;
        indexFile.read(reinterpret_cast<char*>(&x), sizeof(int));
        cout << x << " ";
    }
    cout << endl;
    int data;
    indexFile.seekg(0, ios::beg);
    for (int i = 0; i < size; ++i) {
        indexFile.read(reinterpret_cast<char*>(&data), sizeof(int));
        tmp.write(reinterpret_cast<char*>(&data), sizeof(int));
    }
    indexFile.close();
    tmp.close();
    
    // copy tmp to indexFile
    indexFile.open(filename, ios::out | ios::binary);
    tmp.open("tmp", ios::in | ios::binary);
    
    cout << "tmp file contents : ";
    for (int i = 0; i < size; ++i) {
        int x;
        tmp.read(reinterpret_cast<char*>(&x), sizeof(int));
        cout << x << " ";
    }
    cout << endl;
    
    
    
    indexFile.seekp(0, ios::beg);
    tmp.seekg(0, ios::beg);
    
    // file_header copy
    for (int i = 0; i < 3; ++i) {
        tmp.read(reinterpret_cast<char*>(&data), sizeof(int));
        indexFile.write(reinterpret_cast<char*>(&data), sizeof(int));
        --size;
    }
    // copy before rnode's BID
    for (int i = 1; i < rnode->myBID; ++i) {
        for (int j = 0; j < number_of_data_per_node(blockSize); ++j) {
            tmp.read(reinterpret_cast<char*>(&data), sizeof(int));
            indexFile.write(reinterpret_cast<char*>(&data), sizeof(int));
            --size;
        }
    }
    
    // write rnode on indexFile
//    indexFile.seekp(physical_offset_of_PageID(rnode->myBID, blockSize), ios::beg);
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        indexFile.write(reinterpret_cast<char*>(&rnode->entries[i].key), sizeof(int));
        indexFile.write(reinterpret_cast<char*>(&rnode->entries[i].value), sizeof(int));
    }
    indexFile.write(reinterpret_cast<char*>(&rnode->NextBID), sizeof(int));
    
    cout << "back data : ";
    while (size != 0) {
        tmp.read(reinterpret_cast<char*>(&data), sizeof(int));
        indexFile.write(reinterpret_cast<char*>(&data), sizeof(int));
        cout << data << " ";
        --size;
    }
    cout << endl;
    indexFile.close();
    indexFile.open(filename, ios::in | ios::binary);
    
    indexFile.seekg(0, ios::end);
    size = indexFile.tellg() / 4;
    // copy indexFile to tmp
    indexFile.seekg(0, ios::beg);
    cout << "!!!!END Index_File contents : ";
    for (int i = 0; i < size; ++i) {
        int x;
        indexFile.read(reinterpret_cast<char*>(&x), sizeof(int));
        cout << x << " ";
    }
    cout << endl;
    return true;
}

bool BTree::update_non_leaf_node_on_indexFile(Node *v) {
    cout << "BTree::update_non_leaf_node_on_indexFile(Node *v) : " << v->myBID << endl;
    fstream indexFile(filename, ios::in | ios::out | ios::binary);
    indexFile.seekg(0, ios::end);
    long size = indexFile.tellg() / 4;
    // copy indexFile to tmp
    indexFile.seekg(0, ios::beg);
    cout << "START Index_File contents : ";
    for (int i = 0; i < size; ++i) {
        int x;
        indexFile.read(reinterpret_cast<char*>(&x), sizeof(int));
        cout << x << " ";
    }
    cout << endl;
    indexFile.seekp(physical_offset_of_PageID(v->myBID, blockSize), ios::beg);
    indexFile.write(reinterpret_cast<char*>(&v->NextlevelBID), sizeof(int));
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        indexFile.write(reinterpret_cast<char*>(&v->entries[i].key), sizeof(int));
        indexFile.write(reinterpret_cast<char*>(&v->entries[i].BID), sizeof(int));
    }
    indexFile.close();
    indexFile.open(filename, ios::in | ios::binary);
    
    indexFile.seekg(0, ios::end);
    size = indexFile.tellg() / 4;
    // copy indexFile to tmp
    indexFile.seekg(0, ios::beg);
    cout << "END Index_File contents : ";
    for (int i = 0; i < size; ++i) {
        int x;
        indexFile.read(reinterpret_cast<char*>(&x), sizeof(int));
        cout << x << " ";
    }
    cout << endl;
    return true;
}
bool BTree::update_leaf_node_on_indexFile(Node* v) {
    cout << "BTree::update_leaf_node_on_indexFile(Node* v) : " << v->myBID << endl;
    fstream indexFile(filename, ios::in | ios::out | ios::binary);
    indexFile.seekg(0, ios::end);
    long size = indexFile.tellg() / 4;
    // copy indexFile to tmp
    indexFile.seekg(0, ios::beg);
    cout << "START Index_File contents : ";
    for (int i = 0; i < size; ++i) {
        int x;
        indexFile.read(reinterpret_cast<char*>(&x), sizeof(int));
        cout << x << " ";
    }
    cout << endl;
    indexFile.seekp(physical_offset_of_PageID(v->myBID, blockSize), ios::beg);
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        indexFile.write(reinterpret_cast<char*>(&v->entries[i].key), sizeof(int));
        indexFile.write(reinterpret_cast<char*>(&v->entries[i].value), sizeof(int));
    }
    indexFile.write(reinterpret_cast<char*>(&v->NextBID), sizeof(int));
    indexFile.close();
    indexFile.open(filename, ios::in | ios::binary);
    
    indexFile.seekg(0, ios::end);
    size = indexFile.tellg() / 4;
    // copy indexFile to tmp
    indexFile.seekg(0, ios::beg);
    cout << "END Index_File contents : ";
    for (int i = 0; i < size; ++i) {
        int x;
        indexFile.read(reinterpret_cast<char*>(&x), sizeof(int));
        cout << x << " ";
    }
    cout << endl;
    return true;
}
bool BTree::update_BID_on_indexFile(int BID) {
    cout << "BTree::update_BID_on_indexFile : " << BID << endl;
    fstream indexFile(filename, ios::in | ios::out | ios::binary);
    // have to fix error line 821
    int first_non_leaf_node_BID = find_last_leaf_node_BID() + 1;
    for (int i = BID + 1; i < first_non_leaf_node_BID; ++i) {
        Node* v = find_Node_by_BID(i);
        ++v->NextBID;
        update_leaf_node_on_indexFile(v);
    }
////    if (first_non_leaf_node_BID > rootBID) {
////        return true;
////    }
    indexFile.seekg(physical_offset_of_PageID(BID, blockSize), ios::beg);
    indexFile.seekp(physical_offset_of_PageID(BID, blockSize), ios::beg);
    for (int j = 0; j < rootBID - first_non_leaf_node_BID; ++j) {
        Node *tmp = new Node(blockSize);
        indexFile.read(reinterpret_cast<char*>(&tmp->NextlevelBID), sizeof(int));
        for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
            indexFile.read(reinterpret_cast<char*>(&tmp->entries[i].key), sizeof(int));
            indexFile.read(reinterpret_cast<char*>(&tmp->entries[i].BID), sizeof(int));
        }
        if (tmp->NextlevelBID > first_non_leaf_node_BID)
            ++tmp->NextlevelBID;
        for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
            if (tmp->entries[i].BID > first_non_leaf_node_BID)
                ++tmp->entries[i].BID;
        }
        indexFile.write(reinterpret_cast<char*>(&tmp->NextlevelBID), sizeof(int));
        for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
            indexFile.write(reinterpret_cast<char*>(&tmp->entries[i].key), sizeof(int));
            indexFile.write(reinterpret_cast<char*>(&tmp->entries[i].BID), sizeof(int));
        }
        delete tmp;
    }
    return true;
}
int BTree::find_last_leaf_node_BID () {
    ifstream indexFile(filename, ios::in | ios::binary);
    int last_leaf_node_BID = 0;
    
    int NULL_BID(0), NextBID;
    indexFile.seekg(12 + blockSize - 4, ios::beg); // current node's nextBID poistion
    do {
        indexFile.read(reinterpret_cast<char*>(&NextBID), sizeof(int));
        ++last_leaf_node_BID;
        if (last_leaf_node_BID < rootBID)
            indexFile.seekg(12, ios::cur);
    } while (NextBID != NULL_BID);
    return last_leaf_node_BID;
}

bool BTree::update_parent_entry(Node *parent, Node* child) {
//    if (key == 415) {
//        return true;
//    }
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        if (parent->entries[i].BID == child->myBID) {
            parent->entries[i].key = child->entries[0].key;
            break;
        }
    }
    return true;
}
