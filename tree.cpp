/**
 * 实验：目录树查看器（仿 Linux tree 命令）
 * 学号：__2504020250_  姓名：_唐煜尧_
 * 说明：请补全所有标记为 TODO 的函数体，不要修改其他代码。
 * 目录树查看器（仿 Linux tree 命令）
 * 完整实现版本（C语言，左孩子右兄弟二叉树）
 * 编译：gcc -o tree tree.c -std=c99
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

typedef struct FileNode {
    char *name;
    int isDir;
    struct FileNode *firstChild;
    struct FileNode *nextSibling;
} FileNode;

// 创建新结点
FileNode* createNode(const char *name, int isDir) {
    FileNode *node = (FileNode*)malloc(sizeof(FileNode));
    node->name = strdup(name);
    node->isDir = isDir;
    node->firstChild = NULL;
    node->nextSibling = NULL;
    return node;
}

// 比较函数，用于 qsort 对子项按名称排序
int cmpNode(const void *a, const void *b) {
    FileNode *na = *(FileNode**)a;
    FileNode *nb = *(FileNode**)b;
    return strcmp(na->name, nb->name);
}

// 递归构建目录树
FileNode* buildTree(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return NULL;
    }

    // 提取目录名作为当前结点名
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    FileNode *curDir = createNode(base, 1);

    // 临时收集子结点
    struct dirent *entry;
    FileNode **children = NULL;
    int childCount = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);
        struct stat st;
        if (stat(fullPath, &st) != 0) continue;

        FileNode *child = NULL;
        if (S_ISDIR(st.st_mode)) {
            child = buildTree(fullPath);
        } else if (S_ISREG(st.st_mode)) {
            child = createNode(entry->d_name, 0);
        }
        if (child) {
            children = (FileNode**)realloc(children, (childCount+1) * sizeof(FileNode*));
            children[childCount++] = child;
        }
    }
    closedir(dir);

    // 排序并链接成兄弟链表
    if (childCount > 0) {
        qsort(children, childCount, sizeof(FileNode*), cmpNode);
        curDir->firstChild = children[0];
        for (int i = 0; i < childCount-1; i++) {
            children[i]->nextSibling = children[i+1];
        }
    }
    free(children);
    return curDir;
}

// 树形输出
void printTree(FileNode *node, const char *prefix, int isLast) {
    if (!node) return;
    printf("%s", prefix);
    printf("%s", isLast ? "`-- " : "|-- ");
    printf("%s", node->name);
    if (node->isDir) printf("/");
    printf("\n");

    if (!node->firstChild) return;

    FileNode *child = node->firstChild;
    // 计算孩子总数
    int childNum = 0;
    FileNode *tmp = child;
    while (tmp) { childNum++; tmp = tmp->nextSibling; }
    int idx = 0;
    while (child) {
        int lastChild = (++idx == childNum);
        char newPrefix[1024];
        snprintf(newPrefix, sizeof(newPrefix), "%s%s", prefix, isLast ? "    " : "|   ");
        printTree(child, newPrefix, lastChild);
        child = child->nextSibling;
    }
}

// 统计结点总数
int countNodes(FileNode *root) {
    if (!root) return 0;
    return 1 + countNodes(root->firstChild) + countNodes(root->nextSibling);
}

// 统计叶子结点数（firstChild为空）
int countLeaves(FileNode *root) {
    if (!root) return 0;
    int leaf = (root->firstChild == NULL) ? 1 : 0;
    return leaf + countLeaves(root->firstChild) + countLeaves(root->nextSibling);
}

// 二叉树高度（根深度1，空树0）
int treeHeight(FileNode *root) {
    if (!root) return 0;
    int childH = treeHeight(root->firstChild);
    int siblingH = treeHeight(root->nextSibling);
    int h = childH + 1;
    return (h > siblingH) ? h : siblingH;
}

// 统计目录和文件数
void countDirFile(FileNode *root, int *dirs, int *files) {
    if (!root) return;
    if (root->isDir) (*dirs)++;
    else (*files)++;
    countDirFile(root->firstChild, dirs, files);
    countDirFile(root->nextSibling, dirs, files);
}

// 释放整棵树
void freeTree(FileNode *root) {
    if (!root) return;
    freeTree(root->firstChild);
    freeTree(root->nextSibling);
    free(root->name);
    free(root);
}

// 获取当前工作目录的基本名称
char* getBaseName(void) {
    char *cwd = getcwd(NULL, 0);
    if (!cwd) return strdup(".");
    char *base = strrchr(cwd, '/');
    char *res = base ? strdup(base+1) : strdup(cwd);
    free(cwd);
    return res;
}

int main(int argc, char *argv[]) {
    char targetPath[1024];
    if (argc >= 2) {
        strncpy(targetPath, argv[1], sizeof(targetPath)-1);
        targetPath[sizeof(targetPath)-1] = '\0';
    } else {
        if (getcwd(targetPath, sizeof(targetPath)) == NULL) {
            perror("getcwd");
            return 1;
        }
    }

    int len = strlen(targetPath);
    if (len > 0 && targetPath[len-1] == '/')
        targetPath[len-1] = '\0';

    struct stat st;
    if (stat(targetPath, &st) != 0) {
        perror("stat");
        return 1;
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "错误: %s 不是目录\n", targetPath);
        return 1;
    }

    FileNode *root = buildTree(targetPath);
    if (!root) {
        fprintf(stderr, "无法构建目录树\n");
        return 1;
    }

    // 输出根目录名
    char *displayName = NULL;
    if (argc >= 2) {
        displayName = root->name;
    } else {
        displayName = getBaseName();
    }
    printf("%s/\n", displayName);
    if (argc < 2) free(displayName);

    FileNode *child = root->firstChild;
    int childCount = 0;
    FileNode *tmp = child;
    while (tmp) { childCount++; tmp = tmp->nextSibling; }
    int idx = 0;
    while (child) {
        int isLast = (++idx == childCount);
        printTree(child, "", isLast);
        child = child->nextSibling;
    }

    int dirs = 0, files = 0;
    countDirFile(root, &dirs, &files);
    printf("\n%d 个目录, %d 个文件\n", dirs, files);
    printf("二叉树结点总数: %d\n", countNodes(root));
    printf("叶子结点数: %d\n", countLeaves(root));
    printf("树的高度: %d\n", treeHeight(root));

    freeTree(root);
    return 0;
}
