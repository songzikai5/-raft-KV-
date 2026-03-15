#ifndef _RBTREE_H
#define _RBTREE_H
#include<string>
// // 键值对类型
// // 独热码：注意下面只能选一个！！！
// #define KV_RBTYPE_INT_VOID   0    // int ket, void* value
// #define KV_RBTYPE_CHAR_CHAR  1    // char* key, char* value

#define ENABLE_RBTREE_DEBUG  0  // 是否运行测试代码

// 定义键值对的类型
typedef char* RB_KEY_TYPE;    // key类型
typedef char* RB_VALUE_TYPE;  // value类型


// 定义红黑树单独节点
typedef struct _rbtree_node {
    // 业务相关的性质，key-value
    RB_KEY_TYPE key;      // 键
    RB_VALUE_TYPE value;  // 值
    struct _rbtree_node *left;
    struct _rbtree_node *right;

    struct _rbtree_node *parent;
    
    unsigned char color;  // 不同编译器的无符号性质符号不同，这里加上unsigned减少意外。
} rbtree_node;


// 定义整个红黑树
typedef struct _rbtree{
    struct _rbtree_node *root_node; // 根节点
    struct _rbtree_node *nil_node; // 空节点，也就是叶子节点、根节点的父节点
    int count;  // 节点总数量
} rbtree;
typedef rbtree kv_rbtree_t;

// 初始化
// 参数：kv_rb要传地址
// 返回值：0成功，-1失败
int kv_rbtree_init(kv_rbtree_t* kv_rb);
// 销毁
// 参数：kv_rb要传地址
// 返回值：0成功，-1失败
int kv_rbtree_desy(kv_rbtree_t* kv_rb);
// 插入指令：有就报错，没有就创建
// 返回值：0表示成功、-1表示失败、-2表示已经有key
int kv_rbtree_set(kv_rbtree_t* kv_rb, char** tokens);
// 查找指令
// 返回值：正常返回value，NULL表示没有
char* kv_rbtree_get(kv_rbtree_t* kv_rb, char** tokens);
// 删除指令
// 返回值：0成功，-1失败，-2没有
int kv_rbtree_delete(kv_rbtree_t* kv_rb, char** tokens);
// 计数指令
int kv_rbtree_count(kv_rbtree_t* kv_rb);
// 存在指令
// 返回值：1存在，0没有
int kv_rbtree_exist(kv_rbtree_t* kv_rb, char** tokens);

/*----插入操作----*/
// 红黑树插入
// 返回值：0成功，-1失败
int rbtree_insert(rbtree *T, RB_KEY_TYPE key, RB_VALUE_TYPE value);
// 调整插入新节点后的红黑树，使得红色节点不相邻(平衡性)
void rbtree_insert_fixup(rbtree *T, rbtree_node *cur);

/*----删除操作----*/
// 红黑树删除
void rbtree_delete(rbtree *T, rbtree_node *del);
// 调整删除某节点后的红黑树，使得红色节点不相邻(平衡性)
void rbtree_delete_fixup(rbtree *T, rbtree_node *cur);

/*----查找操作----*/
// 红黑树查找
rbtree_node* rbtree_search(rbtree *T, RB_KEY_TYPE key);


  // 序列化整个红黑树为字符串（用于快照）
std::string rbtree_serialize(rbtree *T);

// 从字符串反序列化重建红黑树
void rbtree_deserialize(rbtree *T, const std::string& data);

#endif