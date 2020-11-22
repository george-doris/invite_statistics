#include <iostream>
#include <string>
#include <algorithm>
#include <vector>

#define MAXVEX 10

using Status = int32_t;

// 结点类型：内结点、叶子结点
enum NODE_TYPE {
    INTERNAL, LEAF
};

// 兄弟结点方向：左兄弟结点、右兄弟结点
enum SIBLING_DIRECTION {
    LEFT, RIGHT
};

//using KeyType = int32_t;                        // 键类型
//using DataType = int32_t;                       // 值类型

const int32_t ORDER = 7;                        // B+树的阶（非根内结点的最小子树个数）
const int32_t MINNUM_KEY = ORDER - 1;           // 最小键值个数
const int32_t MAXNUM_KEY = 2 * ORDER - 1;       // 最大键值个数
const int32_t MINNUM_CHILD = MINNUM_KEY + 1;    // 最小子树个数
const int32_t MAXNUM_CHILD = MAXNUM_KEY + 1;    // 最大子树个数
const int32_t MINNUM_LEAF = MINNUM_KEY;         // 最小叶子结点键值个数
const int32_t MAXNUM_LEAF = MAXNUM_KEY;         // 最大叶子结点键值个数

// 结点基类
template<typename KeyType>
class CNode {
public:
    CNode() {
        setType(LEAF);
        setKeyNum(0);
    }

    virtual ~CNode() {
        setKeyNum(0);
    }

    NODE_TYPE getType() const { return m_Type; }

    void setType(NODE_TYPE type) { m_Type = type; }

    int32_t getKeyNum() const { return m_KeyNum; }

    void setKeyNum(int32_t n) { m_KeyNum = n; }

    KeyType getKeyValue(int32_t i) const { return m_KeyValues[i]; }

    void setKeyValue(int32_t i, KeyType key) { m_KeyValues[i] = key; }

    // 找到键值在结点中存储的下标
    int32_t getKeyIndex(KeyType key) const {
        int32_t left = 0;
        int32_t right = getKeyNum() - 1;
        int32_t current;
        while (left != right) {
            current = (left + right) / 2;
            KeyType currentKey = getKeyValue(current);
            if (key > currentKey) {
                left = current + 1;
            } else {
                right = current;
            }
        }
        return left;
    }

    // 纯虚函数，定义接口
    virtual void removeKey(int32_t keyIndex, int32_t childIndex) = 0;  // 从结点中移除键值
    virtual void split(CNode *parentNode, int32_t childIndex) = 0; // 分裂结点
    virtual void mergeChild(CNode *parentNode, CNode *childNode, int32_t keyIndex) = 0;  // 合并结点
    virtual void clear() = 0; // 清空结点，同时会清空结点所包含的子树结点
    virtual void
    borrowFrom(CNode *destNode, CNode *parentNode, int32_t keyIndex, SIBLING_DIRECTION d) = 0; // 从兄弟结点中借一个键值
    virtual int32_t getChildIndex(KeyType key, int32_t keyIndex) const = 0;  // 根据键值获取孩子结点指针下标
protected:
    NODE_TYPE m_Type;
    int32_t m_KeyNum;
    KeyType m_KeyValues[MAXNUM_KEY];
};

// 内结点
template<typename KeyType>
class CInternalNode : public CNode<KeyType> {
public:
    CInternalNode() : CNode<KeyType>() {
        CNode<KeyType>::setType(INTERNAL);
    }

    virtual ~CInternalNode() {

    }

    CNode<KeyType> *getChild(int32_t i) const { return m_Childs[i]; }

    void setChild(int32_t i, CNode<KeyType> *child) { m_Childs[i] = child; }

    void insert(int32_t keyIndex, int32_t childIndex, KeyType key, CNode<KeyType> *childNode) {
        int32_t i;
        for (i = CNode<KeyType>::getKeyNum(); i > keyIndex; --i)//将父节点中的childIndex后的所有关键字的值和子树指针向后移一位
        {
            setChild(i + 1, m_Childs[i]);
            setKeyValue(i, CNode<KeyType>::m_KeyValues[i - 1]);
        }
        if (i == childIndex) {
            setChild(i + 1, m_Childs[i]);
        }
        setChild(childIndex, childNode);
        setKeyValue(keyIndex, key);
        setKeyNum(CNode<KeyType>::m_KeyNum + 1);
    }

    virtual void split(CNode<KeyType> *parentNode, int32_t childIndex) {
        CInternalNode *newNode = new CInternalNode();   //分裂后的右节点
        newNode->setKeyNum(MINNUM_KEY);
        int32_t i;
        // 拷贝关键字的值
        for (i = 0; i < MINNUM_KEY; ++i) {
            newNode->setKeyValue(i, CNode<KeyType>::m_KeyValues[i + MINNUM_CHILD]);
        }

        // 拷贝孩子节点指针
        for (i = 0; i < MINNUM_CHILD; ++i) {
            newNode->setChild(i, m_Childs[i + MINNUM_CHILD]);
        }
        
        CNode<KeyType>::setKeyNum(MINNUM_KEY);  //更新左子树的关键字个数
        ((CInternalNode *) parentNode)->insert(childIndex, childIndex + 1, CNode<KeyType>::m_KeyValues[MINNUM_KEY], newNode);
    }

    virtual void mergeChild(CNode<KeyType> *parentNode, CNode<KeyType> *childNode, int32_t keyIndex) {
        // 合并数据
        insert(MINNUM_KEY, MINNUM_KEY + 1, parentNode->getKeyValue(keyIndex),
               ((CInternalNode *) childNode)->getChild(0));
        int32_t i;
        for (i = 1; i <= childNode->getKeyNum(); ++i) {
            insert(MINNUM_KEY + i, MINNUM_KEY + i + 1, childNode->getKeyValue(i - 1),
                   ((CInternalNode *) childNode)->getChild(i));
        }
        //父节点删除index的key
        parentNode->removeKey(keyIndex, keyIndex + 1);
        delete ((CInternalNode *) parentNode)->getChild(keyIndex + 1);
    }

    virtual void removeKey(int32_t keyIndex, int32_t childIndex) {
        for (int32_t i = 0; i < CNode<KeyType>::getKeyNum() - keyIndex - 1; ++i) {
            setKeyValue(keyIndex + i, CNode<KeyType>::getKeyValue(keyIndex + i + 1));
            setChild(childIndex + i, getChild(childIndex + i + 1));
        }
        setKeyNum(CNode<KeyType>::getKeyNum() - 1);
    }

    virtual void clear() {
        for (int32_t i = 0; i <= CNode<KeyType>::m_KeyNum; ++i) {
            m_Childs[i]->clear();
            delete m_Childs[i];
            m_Childs[i] = nullptr;
        }
    }

    virtual void borrowFrom(CNode<KeyType> *siblingNode, CNode<KeyType> *parentNode, int32_t keyIndex, SIBLING_DIRECTION d) {
        switch (d) {
            case LEFT:  // 从左兄弟结点借
            {
                insert(0, 0, parentNode->getKeyValue(keyIndex),
                       ((CInternalNode *) siblingNode)->getChild(siblingNode->getKeyNum()));
                parentNode->setKeyValue(keyIndex, siblingNode->getKeyValue(siblingNode->getKeyNum() - 1));
                siblingNode->removeKey(siblingNode->getKeyNum() - 1, siblingNode->getKeyNum());
            }
                break;
            case RIGHT:  // 从右兄弟结点借
            {
                insert(CNode<KeyType>::getKeyNum(), CNode<KeyType>::getKeyNum() + 1, parentNode->getKeyValue(keyIndex),
                       ((CInternalNode *) siblingNode)->getChild(0));
                parentNode->setKeyValue(keyIndex, siblingNode->getKeyValue(0));
                siblingNode->removeKey(0, 0);
            }
                break;
            default:
                break;
        }
    }

    virtual int32_t getChildIndex(KeyType key, int32_t keyIndex) const {
        if (key == CNode<KeyType>::getKeyValue(keyIndex)) {
            return keyIndex + 1;
        } else {
            return keyIndex;
        }
    }

private:
    CNode<KeyType> *m_Childs[MAXNUM_CHILD];
};

// 叶子结点
template<typename KeyType, typename DataType>
class CLeafNode : public CNode<KeyType> {
public:
    CLeafNode() : CNode<KeyType>() {
        CNode<KeyType>::setType(LEAF);
        setLeftSibling(nullptr);
        setRightSibling(nullptr);
    }

    virtual ~CLeafNode() {

    }

    CLeafNode *getLeftSibling() const { return m_LeftSibling; }

    void setLeftSibling(CLeafNode *node) { m_LeftSibling = node; }

    CLeafNode *getRightSibling() const { return m_RightSibling; }

    void setRightSibling(CLeafNode *node) { m_RightSibling = node; }

    DataType getData(int32_t i) const { return m_Datas[i]; }

    void setData(int32_t i, const DataType &data) { m_Datas[i] = data; }

    void insert(KeyType key, const DataType &data) {
        int32_t i;
        for (i = CNode<KeyType>::m_KeyNum; i >= 1 && CNode<KeyType>::m_KeyValues[i - 1] > key; --i) {
            setKeyValue(i, CNode<KeyType>::m_KeyValues[i - 1]);
            setData(i, m_Datas[i - 1]);
        }
        setKeyValue(i, key);
        setData(i, data);
        setKeyNum(CNode<KeyType>::m_KeyNum + 1);
    }

    virtual void split(CNode<KeyType> *parentNode, int32_t childIndex) {
        CLeafNode *newNode = new CLeafNode();//分裂后的右节点
        CNode<KeyType>::setKeyNum(MINNUM_LEAF);
        newNode->setKeyNum(MINNUM_LEAF + 1);
        newNode->setRightSibling(getRightSibling());
        setRightSibling(newNode);
        newNode->setLeftSibling(this);
        int32_t i;
        for (i = 0; i < MINNUM_LEAF + 1; ++i)// 拷贝关键字的值
        {
            newNode->setKeyValue(i, CNode<KeyType>::m_KeyValues[i + MINNUM_LEAF]);
        }
        for (i = 0; i < MINNUM_LEAF + 1; ++i)// 拷贝数据
        {
            newNode->setData(i, m_Datas[i + MINNUM_LEAF]);
        }
        ((CInternalNode<KeyType> *) parentNode)->insert(childIndex, childIndex + 1, CNode<KeyType>::m_KeyValues[MINNUM_LEAF], newNode);
    }

    virtual void mergeChild(CNode<KeyType> *parentNode, CNode<KeyType> *childNode, int32_t keyIndex) {
        // 合并数据
        for (int32_t i = 0; i < childNode->getKeyNum(); ++i) {
            insert(childNode->getKeyValue(i), ((CLeafNode *) childNode)->getData(i));
        }
        setRightSibling(((CLeafNode *) childNode)->getRightSibling());
        //父节点删除index的key，
        parentNode->removeKey(keyIndex, keyIndex + 1);
    }

    virtual void removeKey(int32_t keyIndex, int32_t childIndex) {
        for (int32_t i = keyIndex; i < CNode<KeyType>::getKeyNum() - 1; ++i) {
            setKeyValue(i, CNode<KeyType>::getKeyValue(i + 1));
            setData(i, getData(i + 1));
        }
        setKeyNum(CNode<KeyType>::getKeyNum() - 1);
    }

    virtual void clear() {
//        for (int32_t i = 0; i < m_KeyNum; ++i) {
////            // if type of m_Datas is pointer
////            delete m_Datas[i];
////            m_Datas[i] = nullptr;
//        }
    }

    virtual void borrowFrom(CNode<KeyType> *siblingNode, CNode<KeyType> *parentNode, int32_t keyIndex, SIBLING_DIRECTION d) {
        switch (d) {
            case LEFT:  // 从左兄弟结点借
            {
                insert(siblingNode->getKeyValue(siblingNode->getKeyNum() - 1),
                       ((CLeafNode *) siblingNode)->getData(siblingNode->getKeyNum() - 1));
                siblingNode->removeKey(siblingNode->getKeyNum() - 1, siblingNode->getKeyNum() - 1);
                parentNode->setKeyValue(keyIndex, CNode<KeyType>::getKeyValue(0));
            }
                break;
            case RIGHT:  // 从右兄弟结点借
            {
                insert(siblingNode->getKeyValue(0), ((CLeafNode *) siblingNode)->getData(0));
                siblingNode->removeKey(0, 0);
                parentNode->setKeyValue(keyIndex, siblingNode->getKeyValue(0));
            }
                break;
            default:
                break;
        }
    }

    virtual int32_t getChildIndex(KeyType key, int32_t keyIndex) const {
        return keyIndex;
    }

private:
    CLeafNode *m_LeftSibling;
    CLeafNode *m_RightSibling;
    DataType m_Datas[MAXNUM_LEAF];
};

// 比较操作符：<、<=、=、>=、>、<>
enum COMPARE_OPERATOR {
    LT, LE, EQ, BE, BT, BETWEEN
};

const int32_t INVALID_INDEX = -1;

template<typename KeyType, typename DataType>
struct SelectResult {
    int32_t keyIndex;
    CLeafNode<KeyType, DataType> *targetNode;
};


template<typename KeyType, typename DataType>
class CBPlusTree {
public:
    CBPlusTree() {
        m_Root = nullptr;
        m_DataHead = nullptr;
    }

    ~CBPlusTree() {
        clear();
    }

    bool insert(KeyType key, const DataType &data) {
        // 是否已经存在
        if (search(key)) {
            return false;
        }

        // 找到可以插入的叶子结点，否则创建新的叶子结点
        if (m_Root == nullptr) {
            m_Root = new CLeafNode<KeyType, DataType>();
            m_DataHead = (CLeafNode<KeyType, DataType> *) m_Root;
            m_MaxKey = key;
        }

        if (m_Root->getKeyNum() >= MAXNUM_KEY) // 根结点已满，分裂
        {
            CInternalNode<KeyType> *newNode = new CInternalNode<KeyType>();  //创建新的根节点
            newNode->setChild(0, m_Root);
            m_Root->split(newNode, 0);    // 叶子结点分裂
            m_Root = newNode;  //更新根节点指针
        }

        if (key > m_MaxKey)  // 更新最大键值
        {
            m_MaxKey = key;
        }

        recursive_insert(m_Root, key, data);
        return true;
    }

    bool remove(KeyType key) {
        if (!search(key))  //不存在
        {
            return false;
        }

        if (m_Root->getKeyNum() == 1)//特殊情况处理
        {
            if (m_Root->getType() == LEAF) {
                clear();
                return true;
            } else {
                CNode<KeyType> *pChild1 = ((CInternalNode<KeyType> *) m_Root)->getChild(0);
                CNode<KeyType> *pChild2 = ((CInternalNode<KeyType> *) m_Root)->getChild(1);
                if (pChild1->getKeyNum() == MINNUM_KEY && pChild2->getKeyNum() == MINNUM_KEY) {
                    pChild1->mergeChild(m_Root, pChild2, 0);
                    delete m_Root;
                    m_Root = pChild1;
                }
            }
        }

        recursive_remove(m_Root, key);
        return true;
    }

    bool update(KeyType oldKey, KeyType newKey) {
        if (search(newKey)) // 检查更新后的键是否已经存在
        {
            return false;
        } else {
            int32_t dataValue;
            remove(oldKey, dataValue);
            if (dataValue == INVALID_INDEX) {
                return false;
            } else {
                return insert(newKey, dataValue);
            }
        }
    }

    // 定值查询，compareOperator可以是LT(<)、LE(<=)、EQ(=)、BE(>=)、BT(>)
    std::vector<DataType> select(KeyType compareKey, int32_t compareOpeartor) {
        std::vector<DataType> results;
        if (m_Root != nullptr) {
            if (compareKey > m_MaxKey) {   // 比较键值大于B+树中最大的键值
                if (compareOpeartor == LE || compareOpeartor == LT) {
                    for (CLeafNode<KeyType, DataType> *itr = m_DataHead; itr != nullptr; itr = itr->getRightSibling()) {
                        for (int32_t i = 0; i < itr->getKeyNum(); ++i) {
                            results.push_back(itr->getData(i));
                        }
                    }
                }
            } else if (compareKey < m_DataHead->getKeyValue(0)) {   // 比较键值小于B+树中最小的键值
                if (compareOpeartor == BE || compareOpeartor == BT) {
                    for (CLeafNode<KeyType, DataType> *itr = m_DataHead; itr != nullptr; itr = itr->getRightSibling()) {
                        for (int32_t i = 0; i < itr->getKeyNum(); ++i) {
                            results.push_back(itr->getData(i));
                        }
                    }
                }
            } else {  // 比较键值在B+树中
                SelectResult<KeyType, DataType> result;
                search(compareKey, result);
                switch (compareOpeartor) {
                    case LT:
                    case LE: {
                        CLeafNode<KeyType, DataType> *itr = m_DataHead;
                        int32_t i;
                        while (itr != result.targetNode) {
                            for (i = 0; i < itr->getKeyNum(); ++i) {
                                results.push_back(itr->getData(i));
                            }
                            itr = itr->getRightSibling();
                        }
                        for (i = 0; i < result.keyIndex; ++i) {
                            results.push_back(itr->getData(i));
                        }
                        if (itr->getKeyValue(i) < compareKey ||
                            (compareOpeartor == LE && compareKey == itr->getKeyValue(i))) {
                            results.push_back(itr->getData(i));
                        }
                    }
                        break;
                    case EQ: {
                        if (result.targetNode->getKeyValue(result.keyIndex) == compareKey) {
                            results.push_back(result.targetNode->getData(result.keyIndex));
                        }
                    }
                        break;
                    case BE:
                    case BT: {
                        CLeafNode<KeyType, DataType> *itr = result.targetNode;
                        if (compareKey < itr->getKeyValue(result.keyIndex) ||
                            (compareOpeartor == BE && compareKey == itr->getKeyValue(result.keyIndex))
                                ) {
                            results.push_back(itr->getData(result.keyIndex));
                        }
                        int32_t i;
                        for (i = result.keyIndex + 1; i < itr->getKeyNum(); ++i) {
                            results.push_back(itr->getData(i));
                        }
                        itr = itr->getRightSibling();
                        while (itr != nullptr) {
                            for (i = 0; i < itr->getKeyNum(); ++i) {
                                results.push_back(itr->getData(i));
                            }
                            itr = itr->getRightSibling();
                        }
                    }
                        break;
                    default:  // 范围查询
                        break;
                }
            }
        }
        std::sort<std::vector<DataType>::iterator>(results.begin(), results.end());
        return results;
    }

    // 范围查询，BETWEEN
    std::vector<DataType> select(KeyType smallKey, KeyType largeKey) {
        std::vector<DataType> results;
        if (smallKey <= largeKey) {
            SelectResult<KeyType, DataType> start, end;
            search(smallKey, start);
            search(largeKey, end);
            CLeafNode<KeyType, DataType> *itr = start.targetNode;
            int32_t i = start.keyIndex;
            if (itr->getKeyValue(i) < smallKey) {
                ++i;
            }

            if (end.targetNode->getKeyValue(end.keyIndex) > largeKey) {
                --end.keyIndex;
            }

            while (itr != end.targetNode) {
                for (; i < itr->getKeyNum(); ++i) {
                    results.push_back(itr->getData(i));
                }
                itr = itr->getRightSibling();
                i = 0;
            }

            for (; i <= end.keyIndex; ++i) {
                results.push_back(itr->getData(i));
            }
        }
        std::sort<std::vector<DataType>::iterator>(results.begin(), results.end());
        return results;
    }

    // 查找是否存在
    bool search(KeyType key) {
        return recursive_search(m_Root, key);
    }

    // 清空
    void clear() {
        if (m_Root != nullptr) {
            m_Root->clear();
            delete m_Root;
            m_Root = nullptr;
            m_DataHead = nullptr;
        }
    }

    // 打印树关键字
    void print() const {
        printInConcavo(m_Root, 10);
    }

    // 打印数据
    void printData() const {
        CLeafNode<KeyType, DataType> *itr = m_DataHead;

        while (itr != nullptr) {
            for (int32_t i = 0; i < itr->getKeyNum(); ++i) {
                std::cout << itr->getData(i) << " ";
            }
            std::cout << std::endl;
            itr = itr->getRightSibling();
        }
    }

private:
    void recursive_insert(CNode<KeyType> *parentNode, KeyType key, const DataType &data) {
        // 叶子结点，直接插入
        if (parentNode->getType() == LEAF) {
            ((CLeafNode<KeyType, DataType> *) parentNode)->insert(key, data);
        } else {
            // 找到子结点
            int32_t keyIndex = parentNode->getKeyIndex(key);
            int32_t childIndex = parentNode->getChildIndex(key, keyIndex); // 孩子结点指针索引
            CNode<KeyType> *childNode = ((CInternalNode<KeyType> *) parentNode)->getChild(childIndex);

            // 子结点已满，需进行分裂
            if (childNode->getKeyNum() >= MAXNUM_LEAF) {
                childNode->split(parentNode, childIndex);

                // 确定目标子结点
                if (parentNode->getKeyValue(childIndex) <= key) {
                    childNode = ((CInternalNode<KeyType> *) parentNode)->getChild(childIndex + 1);
                }
            }
            recursive_insert(childNode, key, data);
        }
    }

    void recursive_remove(CNode<KeyType> *parentNode, KeyType key) {
        int32_t keyIndex = parentNode->getKeyIndex(key);
        int32_t childIndex = parentNode->getChildIndex(key, keyIndex); // 孩子结点指针索引

        // 找到目标叶子节点
        if (parentNode->getType() == LEAF) {
            if (key == m_MaxKey && keyIndex > 0) {
                m_MaxKey = parentNode->getKeyValue(keyIndex - 1);
            }

            parentNode->removeKey(keyIndex, childIndex);  // 直接删除

            // 如果键值在内部结点中存在，也要相应的替换内部结点
            if (childIndex == 0 && m_Root->getType() != LEAF && parentNode != m_DataHead) {
                changeKey(m_Root, key, parentNode->getKeyValue(0));
            }
        } else {
            // 内结点
            CNode<KeyType> *pChildNode = ((CInternalNode<KeyType> *) parentNode)->getChild(childIndex); //包含key的子树根节点

            // 包含关键字达到下限值，进行相关操作
            if (pChildNode->getKeyNum() == MINNUM_KEY) {
                // 左兄弟节点
                CNode<KeyType> *pLeft = childIndex > 0 ? ((CInternalNode<KeyType> *) parentNode)->getChild(childIndex - 1)
                                              : nullptr;

                // 右兄弟节点
                CNode<KeyType> *pRight =
                        childIndex < parentNode->getKeyNum() ? ((CInternalNode<KeyType> *) parentNode)->getChild(childIndex + 1)
                                                             : nullptr;

                // 先考虑从兄弟结点中借
                if (pLeft && pLeft->getKeyNum() > MINNUM_KEY) {
                    // 左兄弟结点可借
                    pChildNode->borrowFrom(pLeft, parentNode, childIndex - 1, LEFT);
                } else if (pRight && pRight->getKeyNum() > MINNUM_KEY) {
                    //右兄弟结点可借
                    pChildNode->borrowFrom(pRight, parentNode, childIndex, RIGHT);
                } else if (pLeft) {   //左右兄弟节点都不可借，考虑合并
                    // 与左兄弟合并
                    pLeft->mergeChild(parentNode, pChildNode, childIndex - 1);
                    pChildNode = pLeft;
                } else if (pRight) {
                    //与右兄弟合并
                    pChildNode->mergeChild(parentNode, pRight, childIndex);
                }
            }
            recursive_remove(pChildNode, key);
        }
    }

    void printInConcavo(CNode<KeyType> *pNode, int32_t count) const {
        if (pNode != nullptr) {
            int32_t i, j;
            for (i = 0; i < pNode->getKeyNum(); ++i) {
                if (pNode->getType() != LEAF) {
                    printInConcavo(((CInternalNode<KeyType> *) pNode)->getChild(i), count - 2);
                }
                for (j = count; j >= 0; --j) {
                    std::cout << "-";
                }
                std::cout << pNode->getKeyValue(i) << std::endl;
            }
            if (pNode->getType() != LEAF) {
                printInConcavo(((CInternalNode<KeyType> *) pNode)->getChild(i), count - 2);
            }
        }
    }

    bool recursive_search(CNode<KeyType> *pNode, KeyType key) const {
        // 检测节点指针是否为空，或该节点是否为叶子节点
        if (pNode == nullptr) {
            return false;
        } else {
            int32_t keyIndex = pNode->getKeyIndex(key);
            // 孩子结点指针索引
            int32_t childIndex = pNode->getChildIndex(key, keyIndex);

            if (keyIndex < pNode->getKeyNum() && key == pNode->getKeyValue(keyIndex)) {
                return true;
            } else {
                // 检查该节点是否为叶子节点
                if (pNode->getType() == LEAF) {
                    return false;
                } else {
                    return recursive_search(((CInternalNode<KeyType> *) pNode)->getChild(childIndex), key);
                }
            }
        }
    }

    void changeKey(CNode<KeyType> *pNode, KeyType oldKey, KeyType newKey) {
        if (pNode != nullptr && pNode->getType() != LEAF) {
            int32_t keyIndex = pNode->getKeyIndex(oldKey);
            if (keyIndex < pNode->getKeyNum() && oldKey == pNode->getKeyValue(keyIndex))  // 找到
            {
                pNode->setKeyValue(keyIndex, newKey);
            } else {  // 继续找
                changeKey(((CInternalNode<KeyType> *) pNode)->getChild(keyIndex), oldKey, newKey);
            }
        }
    }

    void search(KeyType key, SelectResult<KeyType, DataType> &result) {
        recursive_search(m_Root, key, result);
    }

    void recursive_search(CNode<KeyType> *pNode, KeyType key, SelectResult<KeyType, DataType> &result) {
        int32_t keyIndex = pNode->getKeyIndex(key);
        int32_t childIndex = pNode->getChildIndex(key, keyIndex); // 孩子结点指针索引
        if (pNode->getType() == LEAF) {
            result.keyIndex = keyIndex;
            result.targetNode = (CLeafNode<KeyType, DataType> *) pNode;
            return;
        } else {
            return recursive_search(((CInternalNode<KeyType> *) pNode)->getChild(childIndex), key, result);
        }
    }

    void remove(KeyType key, DataType &dataValue) {
        // 不存在
        if (!search(key)) {
            dataValue = INVALID_INDEX;
            return;
        }

        // 特殊情况处理
        if (m_Root->getKeyNum() == 1) {
            if (m_Root->getType() == LEAF) {
                dataValue = ((CLeafNode<KeyType, DataType> *) m_Root)->getData(0);
                clear();
                return;
            } else {
                CNode<KeyType> *pChild1 = ((CInternalNode<KeyType> *) m_Root)->getChild(0);
                CNode<KeyType> *pChild2 = ((CInternalNode<KeyType> *) m_Root)->getChild(1);
                if (pChild1->getKeyNum() == MINNUM_KEY && pChild2->getKeyNum() == MINNUM_KEY) {
                    pChild1->mergeChild(m_Root, pChild2, 0);
                    delete m_Root;
                    m_Root = pChild1;
                }
            }
        }

        recursive_remove(m_Root, key, dataValue);
    }

    void recursive_remove(CNode<KeyType> *parentNode, KeyType key, DataType &dataValue) {
        int32_t keyIndex = parentNode->getKeyIndex(key);
        int32_t childIndex = parentNode->getChildIndex(key, keyIndex); // 孩子结点指针索引

        // 找到目标叶子节点
        if (parentNode->getType() == LEAF) {
            if (key == m_MaxKey && keyIndex > 0) {
                m_MaxKey = parentNode->getKeyValue(keyIndex - 1);
            }

            dataValue = ((CLeafNode<KeyType, DataType> *) parentNode)->getData(keyIndex);
            parentNode->removeKey(keyIndex, childIndex);  // 直接删除

            // 如果键值在内部结点中存在，也要相应的替换内部结点
            if (childIndex == 0 && m_Root->getType() != LEAF && parentNode != m_DataHead) {
                changeKey(m_Root, key, parentNode->getKeyValue(0));
            }
        } else {
            // 内结点
            CNode<KeyType> *pChildNode = ((CInternalNode<KeyType> *) parentNode)->getChild(childIndex); //包含key的子树根节点

            // 包含关键字达到下限值，进行相关操作
            if (pChildNode->getKeyNum() == MINNUM_KEY) {
                CNode<KeyType> *pLeft = childIndex > 0 ? ((CInternalNode<KeyType> *) parentNode)->getChild(childIndex - 1)
                                              : nullptr;                       //左兄弟节点
                CNode<KeyType> *pRight =
                        childIndex < parentNode->getKeyNum() ? ((CInternalNode<KeyType> *) parentNode)->getChild(childIndex + 1)
                                                             : nullptr;//右兄弟节点
                // 先考虑从兄弟结点中借
                if (pLeft && pLeft->getKeyNum() > MINNUM_KEY) {
                    // 左兄弟结点可借
                    pChildNode->borrowFrom(pLeft, parentNode, childIndex - 1, LEFT);
                } else if (pRight && pRight->getKeyNum() > MINNUM_KEY) {
                    //右兄弟结点可借
                    pChildNode->borrowFrom(pRight, parentNode, childIndex, RIGHT);
                } else if (pLeft) {
                    //左右兄弟节点都不可借，考虑合并           //与左兄弟合并
                    pLeft->mergeChild(parentNode, pChildNode, childIndex - 1);
                    pChildNode = pLeft;
                } else if (pRight) {
                    //与右兄弟合并
                    pChildNode->mergeChild(parentNode, pRight, childIndex);
                }
            }
            recursive_remove(pChildNode, key, dataValue);
        }
    }

private:
    CNode<KeyType> *m_Root;
    CLeafNode<KeyType, DataType> *m_DataHead;
    KeyType m_MaxKey;  // B+树中的最大键
};

template<typename ElemType>
class ObjArrayList {
private:
    ElemType **arr;    // 指针的指针，一维列表指针
    int32_t length;    // 列表长度
    int32_t size;    // 列表大小
    const int32_t DEFAULT_LENGTH = 10;    // 默认长度
    const int32_t INCREMENT_STEP = 10;    // 增长步长

    // 分配内存空间	0|malloc  1|realloc
    void _AllocSpace(int32_t size = 0, int32_t type = 0) {
        /*
        .	分配内存空间
        .	参数：
        .	int32_t size: 列表大小（元素个数）
        .	int32_t type: 0|malloc  1|realloc
        */
        //新分配的内存起始地址索引
        int32_t start = 0;
        //临时对象数组指针
        ElemType **p = nullptr;
        //1.首次分配内存
        if (type == 0) {
            //this->arr = (ElemType **)malloc(size * sizeof(ElemType *));
            this->arr = new ElemType *[size];    //参考指针数组，保持 new 与 delete 配对
        }
            //2.重新分配内存
        else if (type == 1) {
            //this->arr = (ElemType **)realloc(this->arr, size * sizeof(ElemType *));
            //将原对象数组元素赋值到新数组中，并删除原数组
            p = this->arr;
            this->arr = new ElemType *[size];
            //1.迁移。注此时 this->size 还是原来的大小
            for (int32_t i = 0; i < this->size; i++) {
                this->arr[i] = p[i];
            }
            //2.销毁原数组
            delete[] p;
            //空位起始地址
            start = this->length;
        }
        //3.初始化新分配内存元素为 nullptr
        for (int32_t i = start; i < size; i++) {
            *(this->arr + i) = nullptr;
        }
    }

    // 判断列表是否满
    bool _IsFull() {
        return this->length == this->size ? true : false;
    }

public:
    // 无参构造
    ObjArrayList() {
        _AllocSpace(this->DEFAULT_LENGTH, 0);    // 申请指针列表表空间：默认大小
        this->length = 0;
        this->size = this->DEFAULT_LENGTH;
    }

    // 有参构造
    ObjArrayList(int32_t m) {
        _AllocSpace(m, 0);    //申请指针列表表空间：m 大小
        this->length = 0;
        this->size = m;
    }


    // 析构函数
    ~ObjArrayList() {
        if (this->arr == nullptr) {
            return;
        }
        for (int32_t i = 0; i < this->size; i++) {
            delete *(this->arr + i);
        }
        delete[] this->arr;
    }

    // 获取列表长度
    int32_t Length() {
        return this->length;
    }

    // 获取列表大小
    int32_t Size() {
        return this->size;
    }

    // 顺序增加一个元素
    void Add(ElemType *e) {
        // 重新分配空间大小
        int32_t resize = 0;
        if (_IsFull()) {
            // 顺序增加一个默认步长大小的内存空间
            resize = this->size + this->INCREMENT_STEP;
            _AllocSpace(resize, 1);
            this->size = resize;
        }
        *(this->arr + this->length) = e;
        this->length++;
    }

    // 指定位置增加一个元素
    void Add(int32_t i, ElemType *e) {
        // 重新分配空间大小
        int32_t resize = 0;
        ElemType *p = nullptr;
        // 1.位置 i 在已有列表长度内
        if (i <= this->length - 1) {
            // 若 i 位置已有元素，先销毁原数据内存，再添加新元素
            p = *(this->arr + i);
            if (p != nullptr)
                delete p;
            *(this->arr + i) = e;
        }
            // 2.位置 i 在已有列表长度外，但还在列表大小范围内
        else if (i <= this->size - 1) {
            *(this->arr + i) = e;
            this->length = i + 1;
        }
            // 3.位置 i 在超出列表大小，需重新分配内存
        else {
            // 按 内存增长步长的整数倍， 增加一段新内存
            resize = ((i + 1) / this->INCREMENT_STEP + 1) * this->INCREMENT_STEP;
            _AllocSpace(resize, 1);
            this->size = resize;
            *(this->arr + i) = e;
            this->length = i + 1;
        }
    }

    // 获取指定位置元素
    ElemType *Get(int32_t i) {
        return *(this->arr + i);
    }

    friend std::ostream &operator<<(std::ostream &out, ObjArrayList<ElemType> *list) {
        // 友元函数：重载输出操作符
        std::cout << "The List is : ";
        for (int32_t i = 0; i < list->Length(); i++) {
            std::cout << i << "_";
            if (list->Get(i) != nullptr)
                std::cout << list->Get(i);
            else
                std::cout << "(nullptr) ";
            if (i != list->Length() - 1)
                std::cout << ",";
            if (i % 10 == 9)
                std::cout << std::endl;
        }
        return out;
    }
};

// 全局数组，记录结点是否已补访问
bool visited[MAXVEX];

// 链队列 类模板定义
template<typename ElemType>
class LinkQueue {
    /*
        链队列
    */
    struct Node {
        ElemType *data;
        Node *next;
    };
private:
    Node *front;    //队头指针
    Node *rear;    //队尾指针
public:
    // 无参构造
    LinkQueue() {
        // 1.建立头结点
        Node *p = new Node;
        p->next = nullptr;
        // 2.队头队尾指针指向头结点
        this->rear = p;
        this->front = this->rear;
    }

    // 析构函数：销毁链队
    ~LinkQueue() {
        // 注：采用链头删除法
        Node *p;
        //从front 头结点遍历到链尾
        int32_t i = 0;
        //条件：尾结点 next == nullptr
        while (this->front != nullptr) {
            p = this->front;
            this->front = this->front->next;
            delete p;
            i++;
        }

        std::cout << "共销毁结点：" << i - 1 << "个。" << std::endl;    //计数时，去除头结点
    }


    // 入队
    void EnQueue(ElemType *e) {
        // 1.建立入队结点
        Node *p = new Node;
        p->data = e;
        p->next = nullptr;
        this->rear->next = p;
        // 2.队尾指针后移至入队结点
        this->rear = p;
    }

    // 出队
    ElemType *DeQueue() {
        // 1.空队列判断
        if (this->front == this->rear) {
            std::cout << "空队列！不可出队。" << std::endl;
            return nullptr;
        }
        // 2.非空队时，取队头元素结点
        Node *p = this->front->next;
        ElemType *e = p->data;
        // 3.移除队头元素结点
        this->front->next = p->next;
        delete p;
        // 4.若头结点 next 域为空，即无元素结点，此时将 rear = front
        if (this->front->next == nullptr) {
            this->rear = this->front;
        }
        return e;
    }

    // 获取队头元素
    ElemType *GetHead() {
        // 1.空队列判断
        if (this->front == this->rear) {
            std::cout << "空队列！无队头。" << std::endl;
            return nullptr;
        }
        // 2.非空队时，取队头元素结点
        return this->front->next->data;

    }

    // 获取队尾元素
    ElemType *GetLast() {
        // 1.空队列判断
        if (this->front == this->rear) {
            std::cout << "空队列！无队尾。" << std::endl;
            return nullptr;
        }
        // 2.非空队时，取队尾元素结点
        return this->rear->data;

    }
};

// 链栈 类模板定义
template<typename ElemType>
class LinkStack {
    // 链栈结点
    struct LSNode {
        ElemType *data;    // 数据元素
        LSNode *next;    // 指针域
    };
private:
    LSNode *top;    // 链栈头指针（栈顶指针）
public:
    // 无参构造
    LinkStack() { this->top = nullptr; }

    // 析构函数：销毁链栈
    ~LinkStack() {
        LSNode *p = this->top, *q;
        while (p != nullptr) {
            q = p->next;
            std::cout << "删除：" << p << std::endl;
            delete p->data;
            delete p;
            p = q;
        }
    }

    // 入栈
    void Push(ElemType *e) {
        LSNode *p = new LSNode();
        p->data = e;
        p->next = this->top;
        this->top = p;
    }


    // 出栈
    ElemType *Pop() {
        if (this->top == nullptr) {
            std::cout << "栈空！" << std::endl;
            return nullptr;
        }
        LSNode *p = this->top;
        ElemType *e = this->top->data;
        this->top = this->top->next;
        delete p;
        return e;
    }

    // 获取栈顶元素
    ElemType *GetTop() {
        if (this->top == nullptr) {
            std::cout << "栈空！" << std::endl;
            return nullptr;
        }
        return this->top->data;
    }

    bool Empty() { return this->top == nullptr ? true : false; }
};

/*
.	图（邻接表实现） Graph Adjacency List
.	相关术语：
.		顶点 Vertex ； 边 Edge ；
.		有向图 Digraph ；
.	存储结构：
.		1.顶点表采用B+树结构。
.		2.边表采用B+树结构。
*/
class GraphAdjList {
private:
    // 边表结点
    using EdgeNode = struct EdgeNode {
        int32_t adjVex; // 邻接顶点所在表中下标ID
        EdgeNode *next; // 指向下一条边
    };

    // 顶点表结点
    using VertexNode = struct VertexNode {
        int32_t id; // 顶点ID
        int32_t preID;  // 前向节点
        EdgeNode *pFirstEdge; // 指向第一条边
    };

public:
    // 边数据，注：供外部初始化边数据使用
    using EdgeData = struct EdgeData {
        int32_t Tail;    // 边（弧）尾
        int32_t Head;    // 边（弧）头
    };

public:
    void addInviteRelationship(int32_t preID, int32_t newID) {
        _addVexSet(preID, newID);

        EdgeData * edge = new GraphAdjList::EdgeData{ preID, newID };
        ObjArrayList<EdgeData> * edgesList = new ObjArrayList<EdgeData>();
        edgesList->Add(edge);
        _CreateDG(edgesList);
    }


private:
    static const int32_t _MAX_VERTEX_NUM = 10;          // 支持最大顶点数

    VertexNode vexs[_MAX_VERTEX_NUM];                   // 顶点表
    int32_t vexs_visited[_MAX_VERTEX_NUM];              // 顶点访问标记数组：0|未访问 1|已访问
    int32_t iVexNum; // 顶点个数
    int32_t iEdgeNum; // 边数

    // 创建顶点集合
    void _addVexSet(int32_t preID, int32_t newID) {
        // 顶点是否存在
        if (_Locate(preID) == -1 || _Locate(newID) == -1) {
            this->vexs[newID].id = newID;
            this->vexs[newID].preID = preID;
            this->vexs[newID].pFirstEdge = nullptr;
            this->iVexNum++;
        }
    }

    // 创建有向图
    void _CreateDG(ObjArrayList<EdgeData> *edgesList) {
        // 初始化临时 边对象
        EdgeData * edgeData = nullptr;
        // 初始化 Tail Head 顶点下标索引
        int32_t tail = 0, head = 0;
        // 遍历边数据列表
        for (int32_t i = 0; i < edgesList->Length(); i++) {
            // 按序获取边（弧）
            edgeData = edgesList->Get(i);
            // 定位（或设置）边的两端顶点位置
            tail = _Locate(edgeData->Tail);
            head = _Locate(edgeData->Head);
            // 插入边
            _InsertEdge(tail, head);
        }

    }

    // 定位顶点元素位置
    int32_t _Locate(int32_t vertex) {

        for (int32_t i = 0; i < this->_MAX_VERTEX_NUM; i++) {
            if (vertex == this->vexs[i].id) {
                return i;
            }
        }

        // std::cout << std::endl << "顶点[" << vertex << "]不存在。" << std::endl;
        return -1;

    }

    // 插入边
    void _InsertEdge(int32_t tail, int32_t head) {
        // 边结点指针：初始化为 弧尾 指向的第一个边
        EdgeNode *pEdge = this->vexs[tail].pFirstEdge;
        // 初始化 前一边结点的指针
        EdgeNode *preEdge = nullptr;
        // 重复边布尔值
        bool exist = false;

        // 1.边的重复性校验
        while (pEdge != nullptr) {
            // 若已存在该边，则标记为 存在 true
            if (pEdge->adjVex == head) {
                exist = true;
                break;
            }
            // 若不是该边，继续下一个边校验
            preEdge = pEdge;
            pEdge = pEdge->next;
        }

        // 2.1.如果边存在，则跳过，不做插入
        if (exist)
            return;

        // 2.2.边不存在时，创建边
        EdgeNode *newEdge = new EdgeNode();
        newEdge->adjVex = head;
        newEdge->next = nullptr;

        // 3.1.插入第一条边
        if (preEdge == nullptr) {
            this->vexs[tail].pFirstEdge = newEdge;
        } else {
            // 3.2.插入后序边
            preEdge->next = newEdge;
        }

        // 4.边 计数
        this->iEdgeNum++;
    }

    // 删除边
    void _DeleteEdge(int32_t tail, int32_t head) {
        // 边结点指针：初始化为 弧尾 指向的第一个边
        EdgeNode *p = this->vexs[tail].pFirstEdge;
        // 初始化 前一边结点的指针
        EdgeNode *q = nullptr;

        // 1.遍历查找边
        while (p != nullptr) {
            // 若存在该边，则结束循环
            if (p->adjVex == head) {
                break;
            }
            // 若不是该边，继续下一个边
            q = p;
            p = p->next;
        }

        // 2.1.边不存在
        if (p == nullptr) {
            std::cout << std::endl << "边[" << this->vexs[head].id << "->" << this->vexs[head].id << "]不存在。"
                      << std::endl;
            return;
        }

        // 2.2.边存在，删除边
        // 2.2.1.若为第一条边
        if (q == nullptr) {
            this->vexs[tail].pFirstEdge = p->next;
        }
            // 2.2.2.非第一条边
        else {
            q->next = p->next;
        }
        // 3.释放 p
        delete p;
    }

    // 深度优先遍历 递归
    void _DFS_R(int32_t index) {
        // 1.访问顶点，并标记已访问
        std::cout << this->vexs[index].id << " ";
        this->vexs_visited[index] = 1;

        // 2.遍历访问其相邻顶点
        EdgeNode *p = this->vexs[index].pFirstEdge;
        int32_t adjVex = 0;
        while (p != nullptr) {
            adjVex = p->adjVex;
            p = p->next;
            // 当顶点未被访问过时，可访问
            if (this->vexs_visited[adjVex] != 1) {
                _DFS_R(adjVex);
            }
        }
    }

    // 深度优先遍历 非递归
    void _DFS(int32_t index) {
        // 1.访问第一个结点，并标记为 已访问
        std::cout << this->vexs[index].id << " ";
        vexs_visited[index] = 1;
        // 初始化 边结点 栈
        LinkStack<EdgeNode> *s = new LinkStack<EdgeNode>();
        //初始化边结点 指针
        EdgeNode *pEdge = this->vexs[index].pFirstEdge;
        //2.寻找下一个（未访问的）邻接结点
        while (!s->Empty() || pEdge != NULL) {
            //2.1.未访问过，则访问 （纵向遍历）
            if (vexs_visited[pEdge->adjVex] != 1) {
                //访问结点，标记为访问，并将其入栈
                std::cout << this->vexs[pEdge->adjVex].id << " ";
                vexs_visited[pEdge->adjVex] = 1;
                s->Push(pEdge);
                //指针 p 移向 此结点的第一个邻接结点
                pEdge = this->vexs[pEdge->adjVex].pFirstEdge;
            }
                //2.2.已访问过，移向下一个边结点 （横向遍历）
            else
                pEdge = pEdge->next;
            //3.若无邻接点，则返回上一结点层，并出栈边结点
            if (pEdge == NULL) {
                pEdge = s->Pop();
            }
        }
        //释放 栈
        delete s;
    }

public:
    // 构造函数：初始化图
    GraphAdjList() {
        this->iVexNum = 0;
        this->iEdgeNum = 0;
    }

    // 析构函数
    ~GraphAdjList() {

    }

    // 初始化顶点、边数据为 图|网
    void Init() {
        // 1.创建顶点集
        this->vexs[0].id = 0;
        this->vexs[0].preID = -1;
        this->vexs[0].pFirstEdge = nullptr;
        this->iVexNum++;
    }

    //插入边
    void InsertEdge(const EdgeData &edgeData) {
        // 初始化 Tail Head 顶点下标索引
        int32_t tail = 0, head = 0;
        tail = _Locate(edgeData.Tail);
        head = _Locate(edgeData.Head);
        // 根据图类型，插入边
        _InsertEdge(tail, head);
    }

    // 删除边
    void DeleteEdge(const EdgeData &edgeData) {
        // 初始化 Tail Head 顶点下标索引
        int32_t tail = 0, head = 0;
        tail = _Locate(edgeData.Tail);
        head = _Locate(edgeData.Head);

        // 根据图类型，删除边
        _DeleteEdge(tail, head);
    }

    // 显示 图
    void Display() {
        // 初始化边表结点指针
        EdgeNode *pEdge = nullptr;
        std::cout << std::endl << "邻接表：" << std::endl;

        // 遍历顶点表
        for (int32_t i = 0; i < this->_MAX_VERTEX_NUM; i++) {
            // 空顶点（在删除顶点的操作后会出现此类情况）
            if (this->vexs[i].id == -1) {
                continue;
            }

            // 输出顶点
            std::cout << "[" << i << "]" << this->vexs[i].id << " ";

            // 遍历输出边顶点
            pEdge = this->vexs[i].pFirstEdge;
            while (pEdge != nullptr) {
                std::cout << "[" << pEdge->adjVex << "] ";
                pEdge = pEdge->next;
            }
            std::cout << std::endl;
        }

    }

    // 从指定顶点开始，深度优先 递归 遍历
    void Display_DFS_R(int32_t vertex) {
        // 1.判断顶点是否存在
        int32_t index = _Locate(vertex);
        if (index == -1)
            return;

        // 2.初始化顶点访问数组
        for (int32_t i = 0; i < this->_MAX_VERTEX_NUM; i++) {
            this->vexs_visited[i] = 0;
        }

        // 3.深度优先遍历 递归
        std::cout << "深度优先遍历（递归）：（从顶点" << vertex << "开始）" << std::endl;
        _DFS_R(index);
    }

    // 从指定顶点开始，深度优先 非递归 遍历
    void Display_DFS(int32_t vertex) {
        // 1.判断顶点是否存在
        int32_t index = _Locate(vertex);
        if (index == -1)
            return;

        // 2.初始化顶点访问数组
        for (int32_t i = 0; i < this->_MAX_VERTEX_NUM; i++) {
            this->vexs_visited[i] = 0;
        }

        // 3.深度优先遍历 递归
        std::cout << "深度优先遍历（非递归）：（从顶点" << vertex << "开始）" << std::endl;
        _DFS(index);
    }

    // 从指定顶点开始，广度优先遍历
    void Display_BFS(int32_t vertex) {
        // 1.判断顶点是否存在
        int32_t index = _Locate(vertex);
        if (index == -1)
            return;

        // 2.初始化顶点访问数组
        for (int32_t i = 0; i < this->_MAX_VERTEX_NUM; i++) {
            this->vexs_visited[i] = 0;
        }

        // 3.广度优先遍历
        std::cout << "广度优先遍历：（从顶点" << vertex << "开始）" << std::endl;

        // 3.1.初始化队列
        LinkQueue<int> * vexQ = new LinkQueue<int>();

        // 3.2.访问开始顶点，并标记访问、入队
        std::cout << this->vexs[index].id << " ";
        this->vexs_visited[index] = 1;
        vexQ->EnQueue(new int32_t(index));

        // 3.3.出队，并遍历邻接顶点（下一层次），访问后入队
        EdgeNode *pEdge = nullptr;
        int32_t adjVex = 0;
        while (vexQ->GetHead() != nullptr) {
            index = *vexQ->DeQueue();
            pEdge = this->vexs[index].pFirstEdge;
            // 遍历邻接顶点
            while (pEdge != nullptr) {
                adjVex = pEdge->adjVex;
                // 未访问过的邻接顶点
                if (this->vexs_visited[adjVex] != 1) {
                    // 访问顶点，并标记访问、入队
                    std::cout << this->vexs[adjVex].id << " ";
                    this->vexs_visited[adjVex] = 1;
                    vexQ->EnQueue(new int32_t(adjVex));
                }
                pEdge = pEdge->next;
            }
        }

        // 4.释放队列
        int32_t * e;
        while (vexQ->GetHead() != nullptr)
        {
            e = vexQ->DeQueue();
            delete e;
        }
        delete vexQ;
    }
};

int32_t main() {
    //初始化顶点数据
    int32_t * id0 = new int32_t (0);
    int32_t * id1 = new int32_t(1);
    int32_t * id2 = new int32_t(2);
    int32_t * id3 = new int32_t(3);
    int32_t * id4 = new int32_t(4);
    int32_t * id5 = new int32_t(5);
    int32_t * id6 = new int32_t(6);
    ObjArrayList<int32_t> * vexs = new ObjArrayList<int32_t>();
    vexs->Add(id0);
    vexs->Add(id1);
    vexs->Add(id2);
    vexs->Add(id3);
    vexs->Add(id4);
    vexs->Add(id5);
    vexs->Add(id6);

    //初始化边（弧）数据
    GraphAdjList::EdgeData * edge1 = new GraphAdjList::EdgeData{ 1, 2 };
    GraphAdjList::EdgeData * edge2 = new GraphAdjList::EdgeData{ 1, 3 };
    GraphAdjList::EdgeData * edge3 = new GraphAdjList::EdgeData{ 1, 4 };
    GraphAdjList::EdgeData * edge4 = new GraphAdjList::EdgeData{ 3, 1 };
    GraphAdjList::EdgeData * edge5 = new GraphAdjList::EdgeData{ 3, 2 };
    GraphAdjList::EdgeData * edge6 = new GraphAdjList::EdgeData{ 3, 5 };
    GraphAdjList::EdgeData * edge7 = new GraphAdjList::EdgeData{ 2, 5 };
    GraphAdjList::EdgeData * edge8 = new GraphAdjList::EdgeData{ 4, 6 };
    GraphAdjList::EdgeData * edge9 = new GraphAdjList::EdgeData{ 4, 7 };
    GraphAdjList::EdgeData * edge10 = new GraphAdjList::EdgeData{ 6, 7 };
    ObjArrayList<GraphAdjList::EdgeData> * edgesList = new ObjArrayList<GraphAdjList::EdgeData>();
    edgesList->Add(edge1);
    edgesList->Add(edge2);
    edgesList->Add(edge3);
    edgesList->Add(edge4);
    edgesList->Add(edge5);
    edgesList->Add(edge6);
    edgesList->Add(edge7);
    edgesList->Add(edge8);
    edgesList->Add(edge9);
    edgesList->Add(edge10);

    // 测试1：无向图
    std::cout << std::endl << "图初始化：" << std::endl;
    GraphAdjList *dg = new GraphAdjList();
    dg->Init();

    dg->addInviteRelationship(0, 1);
    dg->addInviteRelationship(1, 2);
    dg->addInviteRelationship(0, 3);
    dg->addInviteRelationship(3, 4);

    dg->Display();

    // 1.1.深度优先遍历
    std::cout << std::endl << "图深度优先遍历序列：（递归）" << std::endl;
    dg->Display_DFS_R(*id0);

    std::cout << std::endl << std::endl << "图深度优先遍历序列：（非递归）" << std::endl;
    dg->Display_DFS(*id0);

    // 1.2.广度优先遍历
    std::cout << std::endl << std::endl << "图广度优先遍历序列：" << std::endl;
    dg->Display_BFS(*id1);

    // 1.3.插入新边
    std::cout << std::endl << std::endl << "新边：" << std::endl;
    dg->addInviteRelationship(3, 5);
    dg->Display();

    // 1.4.删除边
    std::cout << std::endl << std::endl << "无向图删除边arc9：" << std::endl;
//    dg->DeleteEdge(edge9);
    dg->Display();
//
//    // 测试2：有向图
//    std::cout << std::endl << "有向图：" << std::endl;
//    GraphAdjList *dg = new GraphAdjList();
//    dg->Init(vexs, edgesList);
//    dg->Display();
//    // 2.1.深度优先遍历
//    std::cout << std::endl << "有向图深度优先遍历序列：（递归）" << std::endl;
//    dg->Display_DFS_R(id0);
//    std::cout << std::endl << "有向图深度优先遍历序列：（非递归）" << std::endl;
//    dg->Display_DFS(id0);
//    // 2.2.广度优先遍历
//    std::cout << std::endl << "有向图广度优先遍历序列：" << std::endl;
//    dg->Display_BFS(id1);

    return 0;
}
