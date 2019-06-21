//
//  main.cpp
//  DB_proj
//
//  Created by 서형중 on 21/06/2019.
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

enum {LEFT = true, RIGHT = false};
const char *output_filename;

class BTree;
class Node;
class bp_entry {
public:
    bp_entry();
    void init();
private:
    int key;        // key for this entry
    int BID;     // Node pointed this entry
    int value;     // if this entry is in leaf node, pointing data
    
    friend class Node;
    friend class BTree;
};

class Node {
public:
    Node(int blockSize);
    
    bool insert_leaf_entry(int key, int value);
    bool insert_non_leaf_entry(int key, int BID);
    
    bool insertIndexEntry(int key, int BID);
    bool insertDataEntry(int key, int value);
    
    bool IS_LEAF();
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
    int search(int key);   // point search
    int* search(int startRange, int endRange);  // range search
    
private:
    // utility func.
    Node* split(Node *node, int key);       // splitting on leaf node
    Node* _split(Node *node, int key);      // splitting on non leaf node
    
    bool find_leaf_node_by_key(int key, Node **node);
    int find_entry_by_key(Node *node, int key);
    
    // 입력받은 BID에 해당하는 Block의 data를 indexFile에서 read해서 해당 block을 반환.
    Node* find_non_leaf_node_by_BID(int BID);
    Node* find_leaf_node_by_BID(int BID);
    
    // indexFIle 에서 해당 BID 의 Block 을 node 에 읽어옴
    void read_leaf_node_by_BID(Node *target, int BID);
    void read_non_leaf_node_by_BID(Node* target, int BID);
    
    int last_BID();
    void path_init();
    void bp_copy_leaf_node (Node *target_node, Node *node, bool start, int size) ;
    void bp_copy_non_leaf_node(Node *target_node, Node *node, bool start, int size);
    
    // shifting entries to left and return NextlevelBID before shifting
    int bp_node_entry_left_shit(Node *v);
    // update_indexFile
    bool update_file_header_on_indexFile();
    bool update_leaf_node_on_indexFile(Node*);
    bool update_non_leaf_node_on_indexFile(Node*);
    
    fstream indexFile;
    const char *filename;
    int blockSize;  // the physical size of a B_plus-tree node.
    int rootBID;    // the root node's BID in B_plus-tree.bin file.
    int depth;      // check whether a node is leaf or not.
    Node* root;
    
    long long FILE_MAX_BID;
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
                //                for (int i = 0; i < number_of_data_per_node(blockSize); ++i) {
                //                    int tmp = 0;
                //                    indexFile.write(reinterpret_cast<char*>(&tmp), sizeof(int));
                //                }
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
            ofstream output(argv[4], ios::out | ios::trunc);
            if (input.is_open()) {
                while (getline(input, key)) {
                    if (atoi(key.c_str()) != 0) {
                        int rid = myBtree->search(atoi(key.c_str()));
                        output << atoi(key.c_str()) <<  ", " << rid << "\n";
                    }
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
            output_filename = argv[4];
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
                    int size = result[0];
                    for (int i = 1; i < size + 1; ++i) {
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
            output_filename = argv[3];
            myBtree->print();
            break;
        }
            
        default:
            cout << "First option must be one of \"c, i, s, r, p\"\n";
    }
    return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////
bp_entry::bp_entry() {
    key = value = BID = 0;
}

void bp_entry::init() {
    key = value = BID = 0;
}
////////////////////////////////////////////////////////////////////////////////////////////
Node::Node(int blockSize) {
    this->blockSize = blockSize;
    NextBID = NextlevelBID = cnt = parent_BID = myBID = 0;
    this->blockSize = blockSize;
    this->parent = NULL;
    entries = new bp_entry[number_of_entries_per_node(blockSize)];
}

bool Node::insert_leaf_entry(int key, int value) {
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        if (this->entries[i].key == 0) {
            this->entries[i].key = key;
            this->entries[i].value = value;
            ++this->cnt;
            break;
        }
    }
    return true;
}

bool Node::insert_non_leaf_entry(int key, int BID) {
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        if (this->entries[i].key == 0) {
            this->entries[i].key = key;
            this->entries[i].BID = BID;
            ++this->cnt;
            break;
        }
    }
    return true;
}

bool Node::IS_LEAF() {
    return (this->leaf ? true : false);
}

bool Node::IS_FULL() {
    return (this->cnt == number_of_entries_per_node(blockSize) ? true : false);
}
////////////////////////////////////////////////////////////////////////////////////////////
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
        // if root == non-leaf node
    }
    else
        cout << "Error : index file is not successly executed.\n";
    indexFile.close();
    
    if (this->depth > 0)
        read_non_leaf_node_by_BID(this->root, this->rootBID);
    else // if root == leaf node
        read_leaf_node_by_BID(this->root, this->rootBID);
    
    // set root node's status
    root->myBID = rootBID;
    if (this->depth == 0)
        root->leaf = true;
    else
        root->leaf = false;
    // end set root status
}

bool BTree::insert(int key, int rid) {
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
    Node *leafNode;
    bool ret = find_leaf_node_by_key(key, &leafNode); //
    assert(ret != false);
    
    if (leafNode->IS_FULL()) {
        // Splitting a leaf node
        leafNode = split(leafNode, key);
    }
    ret = leafNode->insertDataEntry(key, rid);
    assert(ret != false);
    
    return update_leaf_node_on_indexFile(leafNode);
}

bool BTree::find_leaf_node_by_key(int key, Node **node) {
    // for i : 0 to depth -> depth 만큼 NextlevelNode로 접근해서 해당노드의 BID를
    Node *target = root;
    // find last non-leaf node
    for (int i = 0; i < this->depth - 1; ++i) {
        int nextBID = find_entry_by_key(target, key);
        target = find_non_leaf_node_by_BID(nextBID);
        target->parent = path_root_to_leaf.back();
        path_root_to_leaf.push_back(target);
    }
    // find leaf node
    if (this->depth > 0) {
        int nextBID = find_entry_by_key(target, key);
        target = find_leaf_node_by_BID(nextBID);
        target->parent = path_root_to_leaf.back();
        path_root_to_leaf.push_back(target);
    }
    
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
    return node->entries[number_of_entries_per_node(blockSize) - 1].BID;    // return NULL value. ( 0 == NULL on BID )
}

Node* BTree::find_non_leaf_node_by_BID(int BID) {
    Node *target = new Node(blockSize);
    read_non_leaf_node_by_BID(target, BID);
    return target;
}

Node* BTree::find_leaf_node_by_BID(int BID) {
    Node *target = new Node(blockSize);
    read_leaf_node_by_BID(target, BID);
    return target;
}

void BTree::read_leaf_node_by_BID(Node *target, int BID) {
    ifstream indexFile(filename, ios::in | ios::binary);
    indexFile.seekg(physical_offset_of_PageID(BID, blockSize), ios::beg);
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        int key, value;
        indexFile.read(reinterpret_cast<char*>(&key), sizeof(int));
        indexFile.read(reinterpret_cast<char*>(&value), sizeof(int));
        if (key != 0)
            target->insert_leaf_entry(key, value);
    }
    indexFile.read(reinterpret_cast<char*>(&target->NextBID), sizeof(int));
    target->myBID = BID;
    target->leaf = false;
    indexFile.close();
}

void BTree::read_non_leaf_node_by_BID(Node *target, int BID) {
    ifstream indexFile(filename, ios::in | ios::binary);
    indexFile.seekg(physical_offset_of_PageID(BID, blockSize), ios::beg);
    indexFile.read(reinterpret_cast<char*>(&target->NextlevelBID), sizeof(int));
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        int key, BID;
        indexFile.read(reinterpret_cast<char*>(&key), sizeof(int));
        indexFile.read(reinterpret_cast<char*>(&BID), sizeof(int));
        if (key != 0)
            target->insert_non_leaf_entry(key, BID);
    }
    target->myBID = BID;
    target->leaf = true;
    indexFile.close();
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
    // 해당 노드의 parent 노드를 찾음
    Node *parent = node->parent;
    Node *lnode, *rnode;
    
    lnode = new Node(blockSize);
    rnode = new Node(blockSize);
    // lnode의 BID는 원래 노드의 BID 를 그대로 사용
    // rnode의 BID는 마지막 BID + 1
    if (number_of_entries_per_node(blockSize) % 2 != 0) {
        bp_copy_leaf_node(lnode, node, LEFT, split_factor + 1);
        bp_copy_leaf_node(rnode, node, RIGHT, split_factor);
    }
    else {
        bp_copy_leaf_node(lnode, node, LEFT, split_factor);
        bp_copy_leaf_node(rnode, node, RIGHT, split_factor);
    }
    // lnode, rnode NextBID init
    lnode->NextBID = node->NextBID;
    if (lnode->NextBID != 0)
        rnode->NextBID = lnode->NextBID;
    else
        rnode->NextBID = 0;
    lnode->NextBID = rnode->myBID;
    
    // update lnode on indexFile
    update_leaf_node_on_indexFile(lnode);
    // update rnode on indexFile
    update_leaf_node_on_indexFile(rnode);
    
    if (node == root) { // split node == root -> create new root
        node = new Node(blockSize); // 새로운 root노드 생성
        node->NextlevelBID = lnode->myBID;
        node->myBID = last_BID() + 1;
        node->leaf = false;
        
        // set root, rootBID, depth
        root = node;
        this->rootBID = root->myBID;
        ++this->depth;
        // set lnode, rnode parent
        lnode->parent = node;
        rnode->parent = node;
        lnode->parent_BID = node->myBID;
        rnode->parent_BID = node->myBID;
        parent =  node;
        update_non_leaf_node_on_indexFile(parent);
    }
    else {
        if (parent->IS_FULL()) {
            parent = _split(parent, split_key);
        }
        delete node;
    }
    
    parent->insertIndexEntry(rnode->entries[0].key, rnode->myBID);
    update_non_leaf_node_on_indexFile(parent);
    
    // update file_header
    update_file_header_on_indexFile();
    
    return (key < split_key ? lnode: rnode);
}

void BTree::bp_copy_leaf_node(Node *target_node, Node *node, bool start, int size) {
    target_node->leaf = true;
    if (start == LEFT) {
        target_node->myBID = node->myBID;
        target_node->parent_BID = node->parent_BID;
        target_node->parent = node->parent;
        for (int i = 0; i < size; ++i) {
            target_node->insert_leaf_entry(node->entries[i].key, node->entries[i].value);
        }
    }
    else {
        target_node->myBID = last_BID() + 1;
        target_node->parent_BID = node->parent_BID;
        target_node->parent = node->parent;
        for (int i = size; i < number_of_entries_per_node(blockSize); ++i) {
            target_node->insert_leaf_entry(node->entries[i].key, node->entries[i].value);
        }
    }
}

int BTree::last_BID() {
    ifstream indexFile(filename, ios::in | ios::binary);
    indexFile.seekg(0, ios::end);
    int BID = ((int)indexFile.tellg() - 12) / blockSize;
    indexFile.close();
    return BID;
}

void BTree::path_init() {
    //    for (int i = 1; i < path_root_to_leaf.size(); ++i) {
    //        delete path_root_to_leaf[i];
    //    }
    path_root_to_leaf.clear();
    path_root_to_leaf.push_back(root);
}

bool BTree::update_leaf_node_on_indexFile(Node* v) {
    fstream indexFile(filename, ios::in | ios::out | ios::binary);
    indexFile.seekp(physical_offset_of_PageID(v->myBID, blockSize), ios::beg);
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        indexFile.write(reinterpret_cast<char*>(&v->entries[i].key), sizeof(int));
        indexFile.write(reinterpret_cast<char*>(&v->entries[i].value), sizeof(int));
    }
    indexFile.write(reinterpret_cast<char*>(&v->NextBID), sizeof(int));
    indexFile.close();
    return true;
}

bool BTree::update_non_leaf_node_on_indexFile(Node *v) {
    fstream indexFile(filename, ios::in | ios::out | ios::binary);
    indexFile.seekp(physical_offset_of_PageID(v->myBID, blockSize), ios::beg);
    indexFile.write(reinterpret_cast<char*>(&v->NextlevelBID), sizeof(int));
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        indexFile.write(reinterpret_cast<char*>(&v->entries[i].key), sizeof(int));
        indexFile.write(reinterpret_cast<char*>(&v->entries[i].BID), sizeof(int));
    }
    indexFile.close();
    
    return true;
}

bool BTree::update_file_header_on_indexFile() {
    fstream indexFile(filename, ios::in | ios::out | ios::binary);
    indexFile.seekp(sizeof(int), ios::beg);
    indexFile.write(reinterpret_cast<char*>(&this->rootBID), sizeof(int));
    indexFile.write(reinterpret_cast<char*>(&this->depth), sizeof(int));
    indexFile.close();
    return true;
}

Node* BTree::_split(Node *node, int key) {
    // have to modify list
    // 1. split_factor
    // 2. split 발생시 NextlevelBID 값 처리
    // 3. non-leaf node status 조정
    int split_factor = (number_of_entries_per_node(blockSize) + 1) >> 1;
    int split_key = node->entries[split_factor].key;
    // 해당 노드의 parent 노드를 찾음
    Node *parent = node->parent;
    Node *lnode, *rnode;
    
    lnode = new Node(blockSize);
    rnode = new Node(blockSize);
    // lnode의 BID는 원래 노드의 BID를 그대로 사용
    // rnode의 BID는 마지막 BID + 1
    if (number_of_entries_per_node(blockSize) % 2 != 0) {
        bp_copy_non_leaf_node(lnode, node, LEFT, split_factor + 1);
        bp_copy_non_leaf_node(rnode, node, RIGHT, split_factor);
    }
    else {
        bp_copy_non_leaf_node(lnode, node, LEFT, split_factor);
        bp_copy_non_leaf_node(rnode, node, RIGHT, split_factor);
    }
    // lnode, rnode NextlevelBID init
    lnode->NextlevelBID = node->NextlevelBID;
    bp_node_entry_left_shit(rnode);
    
    // update lnode on indexFile
    update_non_leaf_node_on_indexFile(lnode);
    // update rnode on indexFile
    update_non_leaf_node_on_indexFile(rnode);
    
    // indexFile -> lnode 위치 + 1 부터 write rnode data (BlockSize 만큼)
    if (node == root) { // 첫번째 Split이 발생한다면 Node는 분명 Root임이 틀림없죠!ㅋ
        node = new Node(blockSize); // 새로운 root노드 생성
        node->NextlevelBID = lnode->myBID;
        node->myBID = last_BID() + 1;
        node->leaf = false;
        // have to set new Node's BID
        root = node;
        this->rootBID = root->myBID;
        ++this->depth;
        // set lnode, rnode parent
        lnode->parent = node;
        rnode->parent = node;
        lnode->parent_BID = node->myBID;
        rnode->parent_BID = node->myBID;
        parent = node;
        update_non_leaf_node_on_indexFile(parent);
    }
    else {
        if (parent->IS_FULL()) {
            parent = _split(parent, split_key);
        }
        delete node;
    }
    
    parent->insertIndexEntry(split_key, rnode->myBID);
    update_non_leaf_node_on_indexFile(parent);
    
    // update file_header
    update_file_header_on_indexFile();
    
    return (key < split_key ? lnode: rnode);
}

void BTree::bp_copy_non_leaf_node(Node *target_node, Node *node, bool start, int size) {
    target_node->leaf = false;
    if (start == LEFT) {
        target_node->myBID = node->myBID;
        target_node->parent_BID = node->parent_BID;
        target_node->parent = node->parent;
        for (int i = 0; i < size; ++i) {
            target_node->insert_non_leaf_entry(node->entries[i].key, node->entries[i].BID);
        }
    }
    else {
        target_node->myBID = last_BID() + 1;
        target_node->parent_BID = node->parent_BID;
        target_node->parent = node->parent;
        for (int i = size; i < number_of_entries_per_node(blockSize); ++i) {
            target_node->insert_non_leaf_entry(node->entries[i].key, node->entries[i].BID);
        }
    }
}

int BTree::bp_node_entry_left_shit(Node *v) {
    int tmp_nextlevelBID = v->NextlevelBID;
    v->NextlevelBID = v->entries[0].BID;
    for (int i = 1; i < number_of_entries_per_node(blockSize) - 1; ++i) {
        v->entries[i - 1] = v->entries[i];
    }
    return tmp_nextlevelBID;
}


void BTree::print() {
    ofstream output(output_filename, ios::trunc);
    queue<int> q;
    output << "<0>\n\n";
    if (this->depth > 0) {
        q.push(this->root->NextlevelBID);
        for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
            if (this->root->entries[i].BID != 0) {
                q.push(this->root->entries[i].BID);
                output << this->root->entries[i].key << ", ";
            }
            else break;
        }
    }
    else {
        for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
            if (this->root->entries[i].key != 0) {
                output << this->root->entries[i].value << ", ";
            }
            else {
                output.seekp(-2, ios::cur);
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
            output << " \n\n<" << ++depth << ">\n\n";
        }
        int BID = q.front(); q.pop();
        --breath;
        
        if (depth < this->depth) {
            Node *v = find_non_leaf_node_by_BID(BID);
            q.push(v->NextlevelBID);
            for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
                if (v->entries[i].BID != 0) {
                    q.push(v->entries[i].BID);
                    output << v->entries[i].key << ", ";
                }
            }
            //            output.seekp(-2, ios::end);
            //            output << " / ";
            delete v;
        }
        else {
            Node *v = find_leaf_node_by_BID(BID);
            for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
                if (v->entries[i].value != 0) {
                    output << v->entries[i].key << ", ";
                }
            }
            //            output.seekp(-2, ios::end);
            //            output << " / ";
            delete v;
        }
    }
    output.seekp(-2, ios::end);
    output << " \n";
    output.close();
    return;
}
int BTree::search(int key) { // point search
    Node *target_Node;
    path_init();
    bool ret = find_leaf_node_by_key(key, &target_Node);
    assert(ret != false);
    
    int rid = 0;
    for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
        if (target_Node->entries[i].key == key) {
            rid = target_Node->entries[i].value;
            return rid;
        }
    }
    return rid; // Error : Search key is not exist in B PLUS TREE
}

int* BTree::search(int startRange, int endRange) { // range search
    Node *start_Node;
    path_init();
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
        start_Node = find_leaf_node_by_BID(start_Node->NextBID);
        if (start_Node->NextBID == 0)
            break;
    }
    
    // init return value;
    int *result = new int[result_pair.size() + 1];
    result[0] = (int)result_pair.size();
    for (int i = 0; i < result_pair.size(); ++i) {
        result[i + 1] = result_pair[i];
    }
    return result;
}

bool Node::insertIndexEntry(int key, int BID) {
    bool ret = this->insert_non_leaf_entry(key, BID);
    assert(ret != false);
    for (int i = cnt - 1; i > 0; --i) {
        if (entries[i - 1].key > entries[i].key ) {
            swap(entries[i], entries[i - 1]);
        }
    }
    return true;
}

bool Node::insertDataEntry(int key, int value) {
    bool ret = this->insert_leaf_entry(key, value);
    assert(ret != false);
    for (int i = cnt - 1; i > 0; --i) {
        if (entries[i - 1].key > entries[i].key ) {
            swap(entries[i], entries[i - 1]);
        }
    }
    return true;
}
