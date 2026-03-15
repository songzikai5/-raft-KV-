#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<vector>
#include<stack>
#include <sstream>
#include"rbtree.hpp"
#include "myallocator.h" 

static myallocator<char> g_rb_allocator; 
// 编译指令：gcc -o main 1-1rbtree.c
// 本代码实现红黑树，存储int型key，未指定value。

#define RED   1
#define BLACK 0

// /*-----------------------------函数声明-------------------------------*/
// /*----初始化及释放内存----*/
// // 红黑树初始化，注意调用完后释放内存rbtree_free
// int rbtree_malloc(rbtree *T);
// // 红黑树释放内存
// void rbtree_free(rbtree *T);

// /*----插入操作----*/
// // 红黑树插入
// // 返回值：0成功，-1失败
// int rbtree_insert(rbtree *T, RB_KEY_TYPE key, RB_VALUE_TYPE value);
// // 调整插入新节点后的红黑树，使得红色节点不相邻(平衡性)
// void rbtree_insert_fixup(rbtree *T, rbtree_node *cur);

// /*----删除操作----*/
// // 红黑树删除
// void rbtree_delete(rbtree *T, rbtree_node *del);
// // 调整删除某节点后的红黑树，使得红色节点不相邻(平衡性)
// void rbtree_delete_fixup(rbtree *T, rbtree_node *cur);

// /*----查找操作----*/
// // 红黑树查找
// rbtree_node* rbtree_search(rbtree *T, RB_KEY_TYPE key);

// /*----其他函数----*/
// // 在给定节点作为根节点的子树中，找出key最小的节点
// rbtree_node* rbtree_min(rbtree *T, rbtree_node *cur);
// // 在给定节点作为根节点的子树中，找出key最大的节点
// rbtree_node* rbtree_max(rbtree *T, rbtree_node *cur);
// // 找出当前节点的前驱节点
// rbtree_node* rbtree_precursor_node(rbtree *T, rbtree_node *cur);
// // 找出当前节点的后继节点
// rbtree_node* rbtree_successor_node(rbtree *T, rbtree_node *cur);
// // 红黑树节点左旋，无需修改颜色
// void rbtree_left_rotate(rbtree *T, rbtree_node *x);
// // 红黑树节点右旋，无需修改颜色
// void rbtree_right_rotate(rbtree *T, rbtree_node *y);
// // 计算红黑树的深度
// int rbtree_depth(rbtree *T);
// // 递归计算红黑树的深度（不包括叶子节点）
// int rbtree_depth_recursion(rbtree *T, rbtree_node *cur);
/*-------------------------------------------------------------------*/


/*-----------------------------函数定义-------------------------------*/
// 红黑树初始化，注意调用完后释放内存rbtree_free()
int rbtree_malloc(rbtree *T){
    // rbtree* new_T = (rbtree*)malloc(sizeof(rbtree));
    // if(new_T == NULL){
    //     printf("rbtree malloc failed!");
    //     return -1;
    // }
    rbtree_node* nil_node = (rbtree_node*)g_rb_allocator.allocate(sizeof(rbtree_node));
    nil_node->color = BLACK;
    nil_node->left = nil_node;
    nil_node->right = nil_node;
    nil_node->parent = nil_node;
    T->root_node = nil_node;
    T->nil_node = nil_node;
    T->count = 0;
    return 0;
}

// 红黑树释放内存
void rbtree_free(rbtree *T){
    if(T->nil_node){
        //free(T->nil_node);
        g_rb_allocator.deallocate(T->nil_node, sizeof(rbtree_node));
        T->nil_node = NULL;
    }
}

// 在给定节点作为根节点的子树中，找出key最小的节点
rbtree_node* rbtree_min(rbtree *T, rbtree_node *cur){  
    while(cur->left != T->nil_node){
        cur = cur->left;
    }
    return cur;
}

// 在给定节点作为根节点的子树中，找出key最大的节点
rbtree_node* rbtree_max(rbtree *T, rbtree_node *cur){  
    while(cur->right != T->nil_node){
        cur = cur->right;
    }
    return cur;
}

// 找出当前节点的前驱节点
rbtree_node* rbtree_precursor_node(rbtree *T, rbtree_node *cur){
    // 若当前节点有左孩子，那就直接向下找
    if(cur->left != T->nil_node){
        return rbtree_max(T, cur->left);
    }

    // 若当前节点没有左孩子，那就向上找
    rbtree_node *parent = cur->parent;
    while((parent != T->nil_node) && (cur == parent->left)){
        cur = parent;
        parent = cur->parent;
    }
    return parent;
    // 若返回值为空节点，则说明当前节点就是第一个节点
}

// 找出当前节点的后继节点
rbtree_node* rbtree_successor_node(rbtree *T, rbtree_node *cur){
    // 若当前节点有右孩子，那就直接向下找
    if(cur->right != T->nil_node){
        return rbtree_min(T, cur->right);
    }

    // 若当前节点没有右孩子，那就向上找
    rbtree_node *parent = cur->parent;
    while((parent != T->nil_node) && (cur == parent->right)){
        cur = parent;
        parent = cur->parent;
    }
    return parent;
    // 若返回值为空节点，则说明当前节点就是最后一个节点
}

// 红黑树节点左旋，无需修改颜色
void rbtree_left_rotate(rbtree *T, rbtree_node *x){
    // 传入rbtree*是为了判断节点node的左右子树是否为叶子节点、父节点是否为根节点。
    rbtree_node *y = x->right;
    // 注意红黑树中所有路径都是双向的，两边的指针都要改！
    // 另外，按照如下的修改顺序，无需存储额外的节点。
    x->right = y->left;
    if(y->left != T->nil_node){
        y->left->parent = x;
    }

    y->parent = x->parent;
    if(x->parent == T->nil_node){  // x为根节点
        T->root_node = y;
    }else if(x->parent->left == x){
        x->parent->left = y;
    }else{
        x->parent->right = y;
    }

    y->left = x;
    x->parent = y;
}


// 红黑树节点右旋，无需修改颜色
void rbtree_right_rotate(rbtree *T, rbtree_node *y){
    rbtree_node *x = y->left;
    
    y->left = x->right;
    if(x->right != T->nil_node){
        x->right->parent = y;
    }

    x->parent = y->parent;
    if(y->parent == T->nil_node){
        T->root_node = x;
    }else if(y->parent->left == y){
        y->parent->left = x;
    }else{
        y->parent->right = x;
    }

    x->right = y;
    y->parent = x;
}

// 调整插入新节点后的红黑树，使得红色节点不相邻(平衡性)
void rbtree_insert_fixup(rbtree *T, rbtree_node *cur){
    // 父节点是黑色，无需调整。
    // 父节点是红色，则有如下八种情况。
    while(cur->parent->color == RED){
        // 获取叔节点
        rbtree_node *uncle;
        if(cur->parent->parent->left == cur->parent){
            uncle = cur->parent->parent->right;
        }else{
            uncle = cur->parent->parent->left;
        }

        // 若叔节点为红，只需更新颜色(隐含了四种情况)
        // 循环主要在这里起作用
        if(uncle->color == RED){
            // 叔节点为红色：祖父变红/父变黑/叔变黑、祖父节点成新的当前节点。
            if(uncle->color == RED){
                cur->parent->parent->color = RED;
                cur->parent->color = BLACK;
                uncle->color = BLACK;
                cur = cur->parent->parent;
            }
        }
        // 若叔节点为黑，需要变色+旋转(当前节点相当于祖父节点位置包括四种情况:LL/RR/LR/RL)
        // 下面对四种情况进行判断：都是只执行一次
        else{
            if(cur->parent->parent->left == cur->parent){
                // LL：祖父变红/父变黑、祖父右旋。最后的当前节点应该是原来的当前节点。
                if(cur->parent->left == cur){
                    cur->parent->parent->color = RED;
                    cur->parent->color = BLACK;
                    rbtree_right_rotate(T, cur->parent->parent);
                }
                // LR：祖父变红/父变红/当前变黑、父左旋、祖父右旋。最后的当前节点应该是原来的祖父节点。
                else{
                    cur->parent->parent->color = RED;
                    cur->parent->color = RED;
                    cur->color = BLACK;
                    cur = cur->parent;
                    rbtree_left_rotate(T, cur);
                    rbtree_right_rotate(T, cur->parent->parent);
                }
            }
            else{
                // RL：祖父变红/父变红/当前变黑、父右旋、祖父左旋。最后的当前节点应该是原来的祖父节点。
                if(cur->parent->left == cur){
                    cur->parent->parent->color = RED;
                    cur->parent->color = RED;
                    cur->color = BLACK;
                    cur = cur->parent;
                    rbtree_right_rotate(T, cur);
                    rbtree_left_rotate(T, cur->parent->parent);
                }
                // RR：祖父变红/父变黑、祖父左旋。最后的当前节点应该是原来的当前节点。
                else{
                    cur->parent->parent->color = RED;
                    cur->parent->color = BLACK;
                    rbtree_left_rotate(T, cur->parent->parent);
                }
            }
        }
    }
    
    // 将根节点变为黑色
    T->root_node->color = BLACK;
}

// 插入
// 返回值：0成功，-1失败，-2已经有了
int rbtree_insert(rbtree *T, char* key, char* value){
    // 寻找插入位置（红黑树中序遍历升序）
    rbtree_node *cur = T->root_node;
    rbtree_node *next = T->root_node;
    // 刚插入的位置一定是叶子节点
    while(next != T->nil_node){
        cur = next;

        if(strcmp(key,cur->key) > 0){
            next = cur->right;
        }else if(strcmp(key,cur->key) < 0){
            next = cur->left;
        }else if(strcmp(key,cur->key) == 0){
            return -2;
        }
    }

    // 创建新节点
    rbtree_node *new_node = (rbtree_node*)malloc(sizeof(rbtree_node));
    if(new_node == NULL) return -1;
    // 复制key
    char* kcopy = (char*)malloc(strlen(key)+1);
    if(kcopy == NULL) return -1;
    strncpy(kcopy, key, strlen(key)+1);
    // 复制value
    char* vcopy = (char*)malloc(strlen(value)+1);
    if(vcopy == NULL){
        free(kcopy);
        kcopy = NULL;
        return -1;
    }
    strncpy(vcopy, value, strlen(value)+1);
    new_node->key = kcopy;
    new_node->value = vcopy;
    
    // 将新节点插入到叶子节点
    if(cur == T->nil_node){
        // 若红黑树本身没有节点
        T->root_node = new_node;
    }
    else if(strcmp(new_node->key, cur->key) > 0)
    {
        cur->right = new_node;
    }else{
        cur->left = new_node;
    }
    new_node->parent = cur;
    new_node->left = T->nil_node;
    new_node->right = T->nil_node;
    new_node->color = RED;
    T->count++;

    // 调整红黑树，使得红色节点不相邻
    rbtree_insert_fixup(T, new_node);
    return 0;
}


// 调整删除某节点后的红黑树，使得红色节点不相邻(平衡性)
void rbtree_delete_fixup(rbtree *T, rbtree_node *cur){
    // child是黑色、child不是根节点才会进入循环
    while((cur->color == BLACK) && (cur != T->root_node)){
        // 获取兄弟节点
        rbtree_node *brother = T->nil_node;
        if(cur->parent->left == cur){
            brother = cur->parent->right;
        }else{
            brother = cur->parent->left;
        }
        
        // 兄弟节点为红色：父变红/兄弟变黑、父单旋、当前节点下一循环
        if(brother->color == RED){
            cur->parent->color = RED;
            brother->color = BLACK;
            if(cur->parent->left == cur){
                rbtree_left_rotate(T, cur->parent);
            }else{
                rbtree_right_rotate(T, cur->parent);
            }
        }
        // 兄弟节点为黑色
        else{ 
            // 兄弟节点没有红色子节点：父变黑/兄弟变红、看情况是否结束循环
            if((brother->left->color == BLACK) && (brother->right->color == BLACK)){
                // 若父原先为黑，父节点成新的当前节点进入下一循环；否则结束循环。
                if(brother->parent->color == BLACK){
                    cur = cur->parent;
                }else{
                    cur = T->root_node;
                }
                brother->parent->color = BLACK;
                brother->color = RED;
            }
            // 兄弟节点有红色子节点：LL/LR/RR/RL
            else if(brother->parent->left == brother){
                // LL：红子变黑/兄弟变父色/父变黑、父右旋，结束循环
                if(brother->left->color == RED){
                    brother->left->color = BLACK;
                    brother->color = brother->parent->color;
                    brother->parent->color = BLACK;
                    rbtree_right_rotate(T, brother->parent);
                    cur = T->root_node;
                }
                // LR：红子变父色/父变黑、兄弟左旋/父右旋，结束循环
                else{
                    brother->right->color = brother->parent->color;
                    cur->parent->color = BLACK;
                    rbtree_left_rotate(T, brother);
                    rbtree_right_rotate(T, cur->parent);
                    cur = T->root_node;
                }
            }else{
                // RR：红子变黑/兄弟变父色/父变黑、父左旋，结束循环
                if(brother->right->color == RED){
                    brother->right->color = BLACK;
                    brother->color = brother->parent->color;
                    brother->parent->color = BLACK;
                    rbtree_left_rotate(T, brother->parent);
                    cur = T->root_node;
                }
                // RL：红子变父色/父变黑、兄弟右旋/父左旋，结束循环
                else{
                    brother->left->color = brother->parent->color;
                    brother->parent->color = BLACK;
                    rbtree_right_rotate(T, brother);
                    rbtree_left_rotate(T, cur->parent);
                    cur = T->root_node;
                }
            }
        }
    }
    // 下面这行处理情况2/3
    cur->color = BLACK;
}

// 红黑树删除
void rbtree_delete(rbtree *T, rbtree_node *del){
    if(del != T->nil_node){
        /* 红黑树删除逻辑：
            1. 标准的BST删除操作(本函数)：最红都会转换成删除只有一个子节点或者没有子节点的节点。
            2. 若删除节点为黑色，则进行调整(rebtre_delete_fixup)。
        */
        rbtree_node *del_r = T->nil_node;        // 实际删除的节点
        rbtree_node *del_r_child = T->nil_node;  // 实际删除节点的子节点

        // 找出实际删除的节点
        // 注：实际删除的节点最多只有一个子节点，或者没有子节点(必然在最后两层中，不包括叶子节点那一层)
        if((del->left == T->nil_node) || (del->right == T->nil_node)){
            // 如果要删除的节点本山就只有一个孩子或者没有孩子，那实际删除的节点就是该节点
            del_r = del;
        }else{
            // 如果要删除的节点有两个孩子，那就使用其后继节点(必然最多只有一个孩子)
            del_r = rbtree_successor_node(T, del);
        }

        // 看看删除节点的孩子是谁，没有孩子就是空节点
        if(del_r->left != T->nil_node){
            del_r_child = del_r->left;
        }else{
            del_r_child = del_r->right;
        }

        // 将实际要删除的节点删除
        del_r_child->parent = del_r->parent;  // 若child为空节点，最后再把父节点指向空节点
        if(del_r->parent == T->nil_node){
            T->root_node = del_r_child;
        }else if(del_r->parent->left == del_r){
            del_r->parent->left = del_r_child;
        }else{
            del_r->parent->right = del_r_child;
        }

        // 替换键值对
        if(del != del_r){
            free(del->key);
            del->key = del_r->key;
            free(del->value);
            del->value = del_r->value;
        }

        // 最后看是否需要调整
        if(del_r->color == BLACK){
            rbtree_delete_fixup(T, del_r_child);
        }
        
        // 调整空节点的父节点
        if(del_r_child == T->nil_node){
            del_r_child->parent = T->nil_node;
        }
        free(del_r);
        T->count--;
    }
}

// 查找
rbtree_node* rbtree_search(rbtree *T, char* key){
    rbtree_node *cur = T->root_node;
    while(cur != T->nil_node){
        if(strcmp(cur->key,key) > 0){
            cur = cur->left;
        }else if(strcmp(cur->key,key) < 0){
            cur = cur->right;
        }else if(strcmp(cur->key,key) == 0){
            return cur;
        }
    }
    return T->nil_node;
}

// 中序遍历给定结点为根节点的子树（递归）
void rbtree_traversal_node(rbtree *T, rbtree_node *cur){
    if(cur != T->nil_node){
        rbtree_traversal_node(T, cur->left);
        
        if(cur->color == RED){
            printf("Key:%s\tColor:Red\n", cur->key);
        }else{
            printf("Key:%s\tColor:Black\n", cur->key);
        }
        rbtree_traversal_node(T, cur->right);
    }
}

// 中序遍历整个红黑树
void rbtree_traversal(rbtree *T){
    rbtree_traversal_node(T, T->root_node);
}

// 递归计算红黑树的深度（不包括叶子节点）
int rbtree_depth_recursion(rbtree *T, rbtree_node *cur){
    if(cur == T->nil_node){
        return 0;
    }else{
        int left = rbtree_depth_recursion(T, cur->left);
        int right = rbtree_depth_recursion(T, cur->right);
        return ((left > right) ? left : right) + 1;
    }
}

// 计算红黑树的深度
int rbtree_depth(rbtree *T){
    return rbtree_depth_recursion(T, T->root_node);
}

// 获取输入数字的十进制显示宽度
int decimal_width(int num_in){
    int width = 0;
    while (num_in != 0){
        num_in = num_in / 10;
        width++;
    }
    return width;
}


/*----------------------------kv存储协议-------------------------------*/
// 初始化
// 参数：kv_rb要传地址
// 返回值：0成功，-1失败
int kv_rbtree_init(kv_rbtree_t* kv_rb){
    int ret = rbtree_malloc(kv_rb);
    return ret;
}
// 销毁
// 参数：T要传地址
// 返回值：0成功，-1失败
int kv_rbtree_desy(kv_rbtree_t* kv_rb){
    rbtree_free(kv_rb);
    return 0;
}
// 插入指令：有就报错，没有就创建
// 返回值：0表示成功、-1表示失败、-2表示已经有key
int kv_rbtree_set(rbtree* kv_rb, char** tokens){
    return rbtree_insert(kv_rb, tokens[1], tokens[2]);
}
// 查找指令
// 返回值：自然数表示kv的索引，-1表示没有
char* kv_rbtree_get(rbtree* kv_rb, char** tokens){
    rbtree_node* node = rbtree_search(kv_rb, tokens[1]);
    if(node != kv_rb->nil_node){
        return node->value;
    }else{
        return NULL;
    }
}
// 删除指令
// 返回值：0成功，-1失败，-2没有
int kv_rbtree_delete(rbtree* kv_rb, char** tokens){
    rbtree_node* node = rbtree_search(kv_rb, tokens[1]);
    if(node == NULL){
        return -2;
    }
    rbtree_delete(kv_rb, node);
    return 0;
}
// 计数指令
int kv_rbtree_count(rbtree* kv_rb){
    return kv_rb->count;
}
// 存在指令
// 返回值：1存在，0没有
int kv_rbtree_exist(rbtree* kv_rb, char** tokens){
    return (rbtree_search(kv_rb, tokens[1]) != NULL);
}

std::string rbtree_serialize(rbtree *T) {
    if (!T || T->root_node == T->nil_node) {
        return "";
    }
    std::ostringstream oss;
    
    // 使用栈进行中序遍历（或其他任意顺序，只要遍历所有节点）
    std::stack<rbtree_node*> s;
    rbtree_node* curr = T->root_node;
    while (curr != T->nil_node || !s.empty()) {
        while (curr != T->nil_node) {
            s.push(curr);
            curr = curr->left;
        }
        curr = s.top(); s.pop();
        oss << curr->key << "|" << curr->value << "\n";  // 每行一个 KV
        curr = curr->right;
    }
    return oss.str();
}

// 从字符串反序列化重建红黑树
void rbtree_deserialize(rbtree *T, const std::string& data) {
    // 先清空原树
    kv_rbtree_desy(T);
    kv_rbtree_init(T);

    if (data.empty()) return;

    std::istringstream iss(data);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        size_t pos = line.find('|');
        if (pos == std::string::npos) continue;
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        rbtree_insert(T,
                      const_cast<char*>(key.c_str()),
                      const_cast<char*>(value.c_str()));
    }
}
