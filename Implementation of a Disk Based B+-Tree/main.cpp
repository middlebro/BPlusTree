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

#define physical_offset_of_PageID(BID, blockSize) (12 + (BID - 1) / blockSize)
#define number_of_entries_per_node(blockSize) (blockSize - 4)/ 8
#define number_of_data_per_node(blockSize) blockSize / 4
using namespace std;

class BTree;
class Node;
class bp_entry {
public:
    bp_entry() {
        valid = false;
        key = value = BID = 0;
    }
    void init() {
        valid = false;
        key = value = BID = 0;
    }
private:
    bool valid;     // Is the entry valid?
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
    BTree(const char *filename, int blockSize) {
        // B_plus tree data init.
        indexFile.open(filename, ios::binary);
        if (indexFile.is_open()) {
            indexFile.seekg(0, ios::beg);
            indexFile.read(reinterpret_cast<char*>(&this->blockSize), sizeof(int));
            indexFile.read(reinterpret_cast<char*>(&this->rootBID), sizeof(int));
            indexFile.read(reinterpret_cast<char*>(&this->depth), sizeof(int));
        }
        else
            cout << "Error : index file is not successly executed.\n";
        this->filename = filename;
        
        // root data init.
        root = new Node(blockSize);
        indexFile.seekg(physical_offset_of_PageID(rootBID, blockSize));
        // if root == non-leaf node
        if (depth > 0) {            indexFile.read(reinterpret_cast<char*>(&root->NextlevelBID), sizeof(int));
            for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
                int key, value;
                indexFile.read(reinterpret_cast<char*>(&key), sizeof(int));
                indexFile.read(reinterpret_cast<char*>(&value), sizeof(int));
                root->fill_entry(key, value);
            }

        }
        else { // if root == leaf node
            for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
                int key, value;
                indexFile.read(reinterpret_cast<char*>(&key), sizeof(int));
                indexFile.read(reinterpret_cast<char*>(&value), sizeof(int));
                root->fill_entry(key, value);
            }
            indexFile.read(reinterpret_cast<char*>(&root->NextBID), sizeof(int));
        }
        
        // set root node's status
        root->myBID = rootBID;
        if (depth == 0)
            root->leaf = true;
        else
            root->leaf = false;
        // end set root status
        
        indexFile.close();
    }

    bool insert(int key, int rid) {
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
        Node *leafNode;
        bool ret = find_leaf_node_by_key(key, &leafNode); //
        assert(ret != false);
        
        /*
        if (bp_check_duplicate) {
            cout << "Key is already exist on tree.\n";
            return false;
        }
        */
        if (IS_NODE_FULL(leafNode)) {
            // Splitting a leaf node
            leafNode = split(leafNode, key);
        }
        // overwrite record on leafNode in indexFile
        update_leaf_node_on_indexFile(leafNode);
        
        return leafNode->insertDataEntry(key, rid);
    }
    void print();
    int* search(int key);   // point search
    int* search(int startRange, int endRange);  // range search
    
    // utility func.
    Node* split(Node *node, int key) {
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
        int split_key = node->entries[split_factor + 1].key;
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
        bp_copy_node(lnode, node, LEFT, split_factor + 1);
        bp_copy_node(rnode, node, RIGHT, split_factor);
        
        rnode->myBID = lnode->myBID + 1;
        if (lnode->NextBID != 0)
            rnode->NextBID = lnode->NextBID + 1;
        else
            rnode->NextBID = lnode->NextBID;
        lnode->NextBID = rnode->myBID;
        // indexFile -> lnode 위치 + 1 부터 write rnode data (BlockSize 만큼)
        //
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
            parent = node;
        }
        else {
            if (IS_NODE_FULL(parent)) {
                parent = split(parent, split_key);
            }
            delete node;
        }
        
        // parent 노드에 rnode 추가 <key, BID>
        parent->insertIndexEntry(key, rnode->myBID);
        
        // update lnode on indexFile
        update_lnode_on_indexFile(lnode);
        // update non-leaf node's BID for non-leaf node's NextlevelBID, entry's BID > lnode BID, ++BID
        // (rnode path 오른쪽 Block 들에 대해 ++BID)
        update_BID_on_indexFile(lnode->myBID);
        rootBID = ++root->myBID;
        // update rnode on indexFile
        update_rnode_on_indexFile(rnode);
        // update file_header
        update_file_header_on_indexFile();
        // update root on indexFile
        if (parent == root) {
            update_root_on_indexFile();
        }
        return (split_key < key ? lnode: rnode);
    }
    
    Node* _split(Node *node, int key);
    
    bool find_leaf_node_by_key(int key, Node **node) {
        // for i : 0 to depth -> depth 만큼 NextlevelNode로 접근해서 해당노드의 BID를
        Node *target = root;
        // find last non-leaf node
        for (int i = 0; i < depth - 1; ++i) {
            int nextBID = find_entry_by_key(target, key);
            target = find_Node_by_BID(nextBID);
            target->parent = path_root_to_leaf.back();
            path_root_to_leaf.push_back(target);
        }
        // find leaf node
        if (depth > 0) {
            int nextBID = find_entry_by_key(target, key);
            target = find_leaf_node_by_BID(nextBID);
            target->parent = path_root_to_leaf.back();
            path_root_to_leaf.push_back(target);
        }
        *node = target;
        return true;
    }
    
    int find_entry_by_key(Node *node, int key) {
        // 리프노드가 아닌 경우 해당 키 범위에 해당 되는 다음 레벨 노드의 BID를 반환.
        for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
            if (node->entries[i].key > key) {
                if (i != 0) {
                    return node->entries[i - 1].BID;
                }
                return node->NextlevelBID;
            }
        }
        return 0;    // return NULL value. ( 0 == NULL on BID )
    }
    
    // 입력받은 BID에 해당하는 Block의 data를 indexFile에서 read해서 해당 block을 반환.
    Node* find_Node_by_BID(int BID) {
        ifstream indexFile(filename, ios::in | ios::binary);
        indexFile.seekg(physical_offset_of_PageID(BID, blockSize));
        Node *target = new Node(blockSize);
        indexFile.read(reinterpret_cast<char*>(&target->NextlevelBID), sizeof(int));
        for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
            int key, BID;
            indexFile.read(reinterpret_cast<char*>(&key), sizeof(int));
            indexFile.read(reinterpret_cast<char*>(&BID), sizeof(int));
            target->fill_entry(key, BID);
        }
        target->myBID = BID;
        
        indexFile.close();
        return target;
    }
    Node* find_leaf_node_by_BID(int BID) {
        ifstream indexFile(filename, ios::in | ios::binary);
        indexFile.seekg(physical_offset_of_PageID(BID, blockSize));
        Node *target = new Node(blockSize);
        for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
            int key, BID;
            indexFile.read(reinterpret_cast<char*>(&key), sizeof(int));
            indexFile.read(reinterpret_cast<char*>(&BID), sizeof(int));
            target->fill_entry(key, BID);
        }
        indexFile.read(reinterpret_cast<char*>(&target->NextBID), sizeof(int));
        target->myBID = BID;
        
        indexFile.close();
        return target;
    }
    
    
    
    bool IS_NODE_FULL(Node *v) {
        if (v->IS_FULL())
            return true;
        else
            return false;
    }
    
    bool insert_key(Node *target, int key, int value) {
        return target->insertDataEntry(key, value);
    }
    
    void path_init() {
        path_root_to_leaf.erase(path_root_to_leaf.begin(),path_root_to_leaf.end());
        path_root_to_leaf.push_back(root);
    }
    
    void bp_copy_node (Node *target_node, Node *node, bool start, int size) {
        enum {LEFT = 0, RIGHT = 1};
        if (start == LEFT) {
            for (int i = 0; i < size; ++i) {
               target_node->fill_entry(node->entries[i].key, node->entries[i].value);
            }
        }
        else {
            for (int i = size; i < number_of_entries_per_node(blockSize); ++i) {
                target_node->fill_entry(node->entries[i].key, node->entries[i].value);
            }
        }
    }
    
    // update_indexFile
    void update_file_header_on_indexFile(){
        ofstream indexFile(filename, ios::out | ios::binary);
        indexFile.seekp(sizeof(int), ios::beg);
        indexFile.write(reinterpret_cast<char*>(&this->rootBID), sizeof(int));
        indexFile.write(reinterpret_cast<char*>(&this->depth), sizeof(int));
        indexFile.close();
    }
    void update_root_on_indexFile() {
        indexFile.open(filename, ios::out | ios::binary);
        indexFile.seekp(physical_offset_of_PageID(root->myBID, blockSize));
        for (int i = 0; number_of_entries_per_node(blockSize); ++i) {
            indexFile.write(reinterpret_cast<char*>(&root->entries[i].key), sizeof(int));
            indexFile.write(reinterpret_cast<char*>(&root->entries[i].value), sizeof(int));
        }
        indexFile.write(reinterpret_cast<char*>(&root->NextBID), sizeof(int));
        indexFile.close();
    }
    void update_lnode_on_indexFile(Node* lnode) {
        ofstream indexFile(filename, ios::out | ios::binary);
        indexFile.seekp(physical_offset_of_PageID(lnode->myBID, blockSize));
        for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
            indexFile.write(reinterpret_cast<char*>(&lnode->entries[i].key), sizeof(int));
            indexFile.write(reinterpret_cast<char*>(&lnode->entries[i].value), sizeof(int));
        }
        indexFile.write(reinterpret_cast<char*>(&lnode->NextBID), sizeof(int));
        indexFile.close();
    }
    void update_rnode_on_indexFile(Node* rnode) {
        indexFile.open(filename, ios::app | ios::binary);
        indexFile.seekp(physical_offset_of_PageID(rnode->myBID, blockSize));
        for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
            indexFile.write(reinterpret_cast<char*>(&rnode->entries[i].key), sizeof(int));
            indexFile.write(reinterpret_cast<char*>(&rnode->entries[i].value), sizeof(int));
        }
        indexFile.write(reinterpret_cast<char*>(&rnode->NextBID), sizeof(int));
        indexFile.close();
    }
    void update_leaf_node_on_indexFile(Node* v) {
        indexFile.open(filename, ios::app | ios::binary);
        indexFile.seekp(physical_offset_of_PageID(v->myBID, blockSize));
        for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
            indexFile.write(reinterpret_cast<char*>(&v->entries[i].key), sizeof(int));
            indexFile.write(reinterpret_cast<char*>(&v->entries[i].value), sizeof(int));
        }
        indexFile.write(reinterpret_cast<char*>(&v->NextBID), sizeof(int));
        indexFile.close();
    }
    void update_BID_on_indexFile(int lnode_BID) {
        fstream indexFile(filename, ios::in | ios::out | ios::binary);
        int first_non_leaf_node_BID = find_last_leaf_node_BID() + 1;
        indexFile.seekg(first_non_leaf_node_BID, ios::beg);
        indexFile.seekp(first_non_leaf_node_BID, ios::beg);
        for (int j = 0; j < rootBID - first_non_leaf_node_BID; ++j) {
            Node *tmp = new Node(blockSize);
            indexFile.read(reinterpret_cast<char*>(&tmp->NextlevelBID), sizeof(int));
            for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
                indexFile.read(reinterpret_cast<char*>(&tmp->entries[i].key), sizeof(int));
                indexFile.read(reinterpret_cast<char*>(&tmp->entries[i].BID), sizeof(int));
            }
            if (tmp->NextlevelBID > lnode_BID)
                ++tmp->NextlevelBID;
            for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
                if (tmp->entries[i].BID > lnode_BID)
                    ++tmp->entries[i].BID;
            }
            indexFile.write(reinterpret_cast<char*>(&tmp->NextlevelBID), sizeof(int));
            for (int i = 0; i < number_of_entries_per_node(blockSize); ++i) {
                indexFile.write(reinterpret_cast<char*>(&tmp->entries[i].key), sizeof(int));
                indexFile.write(reinterpret_cast<char*>(&tmp->entries[i].BID), sizeof(int));
            }
            delete tmp;
        }
        
    }
    
    int find_last_leaf_node_BID () {
        ifstream indexFile(filename, ios::in | ios::binary);
        int last_leaf_node_BID = 0;
        
        int NULL_BID(0), NextBID;
        do {
            indexFile.seekg(12 + blockSize * (++last_leaf_node_BID) - 4, ios::beg);
            indexFile.read(reinterpret_cast<char*>(&NextBID), sizeof(int));
        } while (NextBID != NULL_BID);
        return last_leaf_node_BID;
    }
private:
    fstream indexFile;
    const char *filename;
    int blockSize;  // the physical size of a B_plus-tree node.
    int rootBID;    // the root node's BID in B_plus-tree.bin file.
    int depth;      // check whether a node is leaf or not.
    Node* root;
    vector<Node*> path_root_to_leaf;
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
    BTree *myBtree = new BTree(argv[2], atoi(argv[3]));
    switch (command) {
        case 'c': {
            // create index file
            ofstream indexFile(argv[2], ios::out | ios::binary);
            if (indexFile.is_open()) {
                int blockSize = atoi(argv[3]);
                int rootBID = 1;
                int depth = 0;
                indexFile.write(reinterpret_cast<char*>(&blockSize), sizeof(int));
                indexFile.write(reinterpret_cast<char*>(&rootBID), sizeof(int));
                indexFile.write(reinterpret_cast<char*>(&depth), sizeof(int));
            }
            cout << "command : c - create\n";
            indexFile.close();
            break;
        }
        case 'i': {
            // insert recoeds from [records data file], ex) records.txt
            string line;
            ifstream recordFile(argv[3]);
            if (recordFile.is_open()) {
                while (getline(recordFile, line)) {
                    stringstream line_of_recordFile(line);
                    string key, id;
                    getline(line_of_recordFile, key, ',');
                    getline(line_of_recordFile, id, ',');
                    // insert key,
                }
            }
            else
                cout << "Cannot find File : " << argv[3] << " in this directory. \n";
            break;
        }
        case 's': {
            // Point(exact) search
            // search keys in [input file] and print results to [output file]
            string key;
            ifstream inputFile(argv[3]);
            if (inputFile.is_open()) {
                while (getline(inputFile, key)) {

                }
            }

            break;
        }
        case 'r': {
            // Range search
            // search keys in [input file] and print results to [output file]
            string line;
            ifstream inputFile(argv[3]);
            if (inputFile.is_open()) {
                while (getline(inputFile, line)) {
                    stringstream line_of_inputFile(line);
                    string startRange, endRange;
                    getline(line_of_inputFile, startRange, ',');
                    getline(line_of_inputFile, endRange, ',');
                }
            }
            else
                cout << "Cannot find File : " << argv[3] << " in this directory. \n";
            break;
        }
        case 'p': {
            // print (B+)-Tree structure to [output file]

            break;
        }

        default:
            cout << "First option must be one of \"c, i, s, r, p\"\n";
    }

    return 0;
}

Node::Node(int blockSize) {
    NextBID = NextlevelBID = cnt = parent_BID = myBID = 0;
    this->blockSize = blockSize;
    this->parent = NULL;
    entries = new bp_entry[number_of_entries_per_node(blockSize)];
}
bool Node::fill_entry(int key, int value) {
    entries[cnt].key = key;
    entries[cnt].value = value;
    ++cnt;
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
    if (IS_FULL()) {
        // split!
    }
    else {
        bool ret = this->fill_entry(key, BID);
        assert(ret != false);
        for (int i = cnt - 1; i > 0; --i) {
            if (entries[i].key > entries[i - 1].key ) {
                swap(entries[i], entries[i - 1]);
            }
        }
        return true;
    }
    return false;
}
bool Node::insertDataEntry(int key, int value) {
    if (IS_FULL()) {
        // Error : not excuted split!
        return false;
    }
    else {
        bool ret = this->fill_entry(key, value);
        assert(ret != false);
        for (int i = cnt - 1; i > 0; --i) {
            if (entries[i].key > entries[i - 1].key ) {
                swap(entries[i], entries[i - 1]);
            }
        }
        return true;
    }
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
    return (cnt < number_of_entries_per_node(blockSize)) ? false : true;
    }


Node* BTree::_split(Node *node, int key) {
    // have to modify list
    // 1. split_factor
    // 2. split 발생시 NextlevelBID 값 처리
    // 3. non-leaf node status 조정
    int split_factor = (number_of_entries_per_node(blockSize) + 1) >> 1;
    int split_key = node->entries[split_factor + 1].key;
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
    bp_copy_node(lnode, node, LEFT, split_factor + 1);
    bp_copy_node(rnode, node, RIGHT, split_factor);
    
    rnode->myBID = lnode->myBID + 1;
    if (lnode->NextBID != 0)
        rnode->NextBID = lnode->NextBID + 1;
    else
        rnode->NextBID = lnode->NextBID;
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
        parent = node;
    }
    else {
        if (IS_NODE_FULL(parent)) {
            parent = split(parent, split_key);
        }
        delete node;
    }

    // parent 노드에 rnode 추가 <key, BID>
    parent->insertIndexEntry(key, rnode->myBID);
    
    // update lnode on indexFile
    update_lnode_on_indexFile(lnode);
    // update non-leaf node's BID for non-leaf node's NextlevelBID, entry's BID > lnode BID, ++BID
    update_BID_on_indexFile(lnode->myBID);
    rootBID = ++root->myBID;
    // update rnode on indexFile
    update_rnode_on_indexFile(rnode);
    // update file_header
    update_file_header_on_indexFile();
    // update root on indexFile
    if (parent == root) {
        update_root_on_indexFile();
    }
    return (split_key < key ? lnode: rnode);
    }
