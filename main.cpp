#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <list>

#define MAXVEX 10

using Status = int32_t;

//using KeyType = int32_t;                        // 键类型
//using DataType = int32_t;                       // 值类型

const int32_t ORDER = 7;                        // B+树的阶（非根内结点的最小子树个数）
const int32_t MINNUM_KEY = ORDER - 1;           // 最小键值个数
const int32_t MAXNUM_KEY = 2 * ORDER - 1;       // 最大键值个数
const int32_t MINNUM_CHILD = MINNUM_KEY + 1;    // 最小子树个数
const int32_t MAXNUM_CHILD = MAXNUM_KEY + 1;    // 最大子树个数
const int32_t MINNUM_LEAF = MINNUM_KEY;         // 最小叶子结点键值个数
const int32_t MAXNUM_LEAF = MAXNUM_KEY;         // 最大叶子结点键值个数

// 结点类型：内结点、叶子结点
enum NODE_TYPE {
    INTERNAL, LEAF
};

// 兄弟结点方向：左兄弟结点、右兄弟结点
enum SIBLING_DIRECTION {
    LEFT, RIGHT
};

// 结点基类
template<typename KeyType>
class BaseNode {
public:
    BaseNode() {
        setType(LEAF);
        setKeyNum(0);
    }

    virtual ~BaseNode() {
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
    virtual void split(BaseNode *parentNode, int32_t childIndex) = 0; // 分裂结点
    virtual void mergeChild(BaseNode *parentNode, BaseNode *childNode, int32_t keyIndex) = 0;  // 合并结点
    virtual void clear() = 0; // 清空结点，同时会清空结点所包含的子树结点
    virtual void
    borrowFrom(BaseNode *destNode, BaseNode *parentNode, int32_t keyIndex, SIBLING_DIRECTION d) = 0; // 从兄弟结点中借一个键值
    virtual int32_t getChildIndex(KeyType key, int32_t keyIndex) const = 0;  // 根据键值获取孩子结点指针下标
protected:
    NODE_TYPE m_Type;
    int32_t m_KeyNum;
    KeyType m_KeyValues[MAXNUM_KEY];
};

// 内结点
template<typename KeyType>
class InternalNode : public BaseNode<KeyType> {
public:
    InternalNode() : BaseNode<KeyType>() {
        BaseNode<KeyType>::setType(INTERNAL);
    }

    virtual ~InternalNode() {

    }

    BaseNode<KeyType> *getChild(int32_t i) const { return m_Childs[i]; }

    void setChild(int32_t i, BaseNode<KeyType> *child) { m_Childs[i] = child; }

    void insert(int32_t keyIndex, int32_t childIndex, KeyType key, BaseNode<KeyType> *childNode) {
        int32_t i;
        for (i = BaseNode<KeyType>::getKeyNum(); i > keyIndex; --i)//将父节点中的childIndex后的所有关键字的值和子树指针向后移一位
        {
            setChild(i + 1, m_Childs[i]);
            this->setKeyValue(i, BaseNode<KeyType>::m_KeyValues[i - 1]);
        }
        if (i == childIndex) {
            setChild(i + 1, m_Childs[i]);
        }
        setChild(childIndex, childNode);
        this->setKeyValue(keyIndex, key);
        this->setKeyNum(BaseNode<KeyType>::m_KeyNum + 1);
    }

    virtual void split(BaseNode<KeyType> *parentNode, int32_t childIndex) {
        InternalNode *newNode = new InternalNode();   //分裂后的右节点
        newNode->setKeyNum(MINNUM_KEY);
        int32_t i;
        // 拷贝关键字的值
        for (i = 0; i < MINNUM_KEY; ++i) {
            newNode->setKeyValue(i, BaseNode<KeyType>::m_KeyValues[i + MINNUM_CHILD]);
        }

        // 拷贝孩子节点指针
        for (i = 0; i < MINNUM_CHILD; ++i) {
            newNode->setChild(i, m_Childs[i + MINNUM_CHILD]);
        }

        BaseNode<KeyType>::setKeyNum(MINNUM_KEY);  //更新左子树的关键字个数
        ((InternalNode *) parentNode)->insert(childIndex, childIndex + 1, BaseNode<KeyType>::m_KeyValues[MINNUM_KEY], newNode);
    }

    virtual void mergeChild(BaseNode<KeyType> *parentNode, BaseNode<KeyType> *childNode, int32_t keyIndex) {
        // 合并数据
        insert(MINNUM_KEY, MINNUM_KEY + 1, parentNode->getKeyValue(keyIndex),
               ((InternalNode *) childNode)->getChild(0));
        int32_t i;
        for (i = 1; i <= childNode->getKeyNum(); ++i) {
            insert(MINNUM_KEY + i, MINNUM_KEY + i + 1, childNode->getKeyValue(i - 1),
                   ((InternalNode *) childNode)->getChild(i));
        }
        //父节点删除index的key
        parentNode->removeKey(keyIndex, keyIndex + 1);
        delete ((InternalNode *) parentNode)->getChild(keyIndex + 1);
    }

    virtual void removeKey(int32_t keyIndex, int32_t childIndex) {
        for (int32_t i = 0; i < BaseNode<KeyType>::getKeyNum() - keyIndex - 1; ++i) {
            this->setKeyValue(keyIndex + i, BaseNode<KeyType>::getKeyValue(keyIndex + i + 1));
            setChild(childIndex + i, getChild(childIndex + i + 1));
        }
        this->setKeyNum(BaseNode<KeyType>::getKeyNum() - 1);
    }

    virtual void clear() {
        for (int32_t i = 0; i <= BaseNode<KeyType>::m_KeyNum; ++i) {
            m_Childs[i]->clear();
            delete m_Childs[i];
            m_Childs[i] = nullptr;
        }
    }

    virtual void borrowFrom(BaseNode<KeyType> *siblingNode, BaseNode<KeyType> *parentNode, int32_t keyIndex, SIBLING_DIRECTION d) {
        switch (d) {
            case LEFT:  // 从左兄弟结点借
            {
                insert(0, 0, parentNode->getKeyValue(keyIndex),
                       ((InternalNode *) siblingNode)->getChild(siblingNode->getKeyNum()));
                parentNode->setKeyValue(keyIndex, siblingNode->getKeyValue(siblingNode->getKeyNum() - 1));
                siblingNode->removeKey(siblingNode->getKeyNum() - 1, siblingNode->getKeyNum());
            }
                break;
            case RIGHT:  // 从右兄弟结点借
            {
                insert(BaseNode<KeyType>::getKeyNum(), BaseNode<KeyType>::getKeyNum() + 1, parentNode->getKeyValue(keyIndex),
                       ((InternalNode *) siblingNode)->getChild(0));
                parentNode->setKeyValue(keyIndex, siblingNode->getKeyValue(0));
                siblingNode->removeKey(0, 0);
            }
                break;
            default:
                break;
        }
    }

    virtual int32_t getChildIndex(KeyType key, int32_t keyIndex) const {
        if (key == BaseNode<KeyType>::getKeyValue(keyIndex)) {
            return keyIndex + 1;
        } else {
            return keyIndex;
        }
    }

private:
    BaseNode<KeyType> *m_Childs[MAXNUM_CHILD];
};

// 叶子结点
template<typename KeyType, typename DataType>
class LeafNode : public BaseNode<KeyType> {
public:
    LeafNode() : BaseNode<KeyType>() {
        BaseNode<KeyType>::setType(LEAF);
        setLeftSibling(nullptr);
        setRightSibling(nullptr);
    }

    virtual ~LeafNode() {

    }

    LeafNode *getLeftSibling() const { return m_LeftSibling; }

    void setLeftSibling(LeafNode *node) { m_LeftSibling = node; }

    LeafNode *getRightSibling() const { return m_RightSibling; }

    void setRightSibling(LeafNode *node) { m_RightSibling = node; }

    DataType getData(int32_t i) const { return m_Datas[i]; }

    void setData(int32_t i, const DataType &data) { m_Datas[i] = data; }

    void insert(KeyType key, const DataType &data) {
        int32_t i;
        for (i = BaseNode<KeyType>::m_KeyNum; i >= 1 && BaseNode<KeyType>::m_KeyValues[i - 1] > key; --i) {
            this->setKeyValue(i, BaseNode<KeyType>::m_KeyValues[i - 1]);
            setData(i, m_Datas[i - 1]);
        }
        this->setKeyValue(i, key);
        setData(i, data);
        this->setKeyNum(BaseNode<KeyType>::m_KeyNum + 1);
    }

    virtual void split(BaseNode<KeyType> *parentNode, int32_t childIndex) {
        LeafNode *newNode = new LeafNode();//分裂后的右节点
        BaseNode<KeyType>::setKeyNum(MINNUM_LEAF);
        newNode->setKeyNum(MINNUM_LEAF + 1);
        newNode->setRightSibling(getRightSibling());
        setRightSibling(newNode);
        newNode->setLeftSibling(this);
        int32_t i;
        for (i = 0; i < MINNUM_LEAF + 1; ++i)// 拷贝关键字的值
        {
            newNode->setKeyValue(i, BaseNode<KeyType>::m_KeyValues[i + MINNUM_LEAF]);
        }
        for (i = 0; i < MINNUM_LEAF + 1; ++i)// 拷贝数据
        {
            newNode->setData(i, m_Datas[i + MINNUM_LEAF]);
        }
        ((InternalNode<KeyType> *) parentNode)->insert(childIndex, childIndex + 1, BaseNode<KeyType>::m_KeyValues[MINNUM_LEAF], newNode);
    }

    virtual void mergeChild(BaseNode<KeyType> *parentNode, BaseNode<KeyType> *childNode, int32_t keyIndex) {
        // 合并数据
        for (int32_t i = 0; i < childNode->getKeyNum(); ++i) {
            insert(childNode->getKeyValue(i), ((LeafNode *) childNode)->getData(i));
        }
        setRightSibling(((LeafNode *) childNode)->getRightSibling());
        //父节点删除index的key，
        parentNode->removeKey(keyIndex, keyIndex + 1);
    }

    virtual void removeKey(int32_t keyIndex, int32_t childIndex) {
        for (int32_t i = keyIndex; i < BaseNode<KeyType>::getKeyNum() - 1; ++i) {
            this->setKeyValue(i, BaseNode<KeyType>::getKeyValue(i + 1));
            setData(i, getData(i + 1));
        }
        this->setKeyNum(BaseNode<KeyType>::getKeyNum() - 1);
    }

    virtual void clear() {
//        for (int32_t i = 0; i < m_KeyNum; ++i) {
////            // if type of m_Datas is pointer
////            delete m_Datas[i];
////            m_Datas[i] = nullptr;
//        }
    }

    virtual void borrowFrom(BaseNode<KeyType> *siblingNode, BaseNode<KeyType> *parentNode, int32_t keyIndex, SIBLING_DIRECTION d) {
        switch (d) {
            case LEFT:  // 从左兄弟结点借
            {
                insert(siblingNode->getKeyValue(siblingNode->getKeyNum() - 1),
                       ((LeafNode *) siblingNode)->getData(siblingNode->getKeyNum() - 1));
                siblingNode->removeKey(siblingNode->getKeyNum() - 1, siblingNode->getKeyNum() - 1);
                parentNode->setKeyValue(keyIndex, BaseNode<KeyType>::getKeyValue(0));
            }
                break;
            case RIGHT:  // 从右兄弟结点借
            {
                insert(siblingNode->getKeyValue(0), ((LeafNode *) siblingNode)->getData(0));
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
    LeafNode *m_LeftSibling;
    LeafNode *m_RightSibling;
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
    LeafNode<KeyType, DataType> *targetNode;
};

template<typename KeyType, typename DataType>
class BPlusTree {
public:
    BPlusTree() {
        m_Root = nullptr;
        m_DataHead = nullptr;
    }

    ~BPlusTree() {
        clear();
    }

    bool insert(KeyType key, const DataType &data) {
        // 找到可以插入的叶子结点，否则创建新的叶子结点
        if (m_Root == nullptr) {
            m_Root = new LeafNode<KeyType, DataType>();
            m_DataHead = (LeafNode<KeyType, DataType> *) m_Root;
            m_MaxKey = key;
        }

        if (m_Root->getKeyNum() >= MAXNUM_KEY) // 根结点已满，分裂
        {
            InternalNode<KeyType> *newNode = new InternalNode<KeyType>();  //创建新的根节点
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
                BaseNode<KeyType> *pChild1 = ((InternalNode<KeyType> *) m_Root)->getChild(0);
                BaseNode<KeyType> *pChild2 = ((InternalNode<KeyType> *) m_Root)->getChild(1);
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
    std::vector<DataType> select(KeyType compareKey, COMPARE_OPERATOR compareOpeartor) {
        std::vector<DataType> results;
        if (m_Root != nullptr) {
            if (compareKey > m_MaxKey) {   // 比较键值大于B+树中最大的键值
                if (compareOpeartor == LE || compareOpeartor == LT) {
                    for (LeafNode<KeyType, DataType> *itr = m_DataHead; itr != nullptr; itr = itr->getRightSibling()) {
                        for (int32_t i = 0; i < itr->getKeyNum(); ++i) {
                            results.push_back(itr->getData(i));
                        }
                    }
                }
            } else if (compareKey < m_DataHead->getKeyValue(0)) {   // 比较键值小于B+树中最小的键值
                if (compareOpeartor == BE || compareOpeartor == BT) {
                    for (LeafNode<KeyType, DataType> *itr = m_DataHead; itr != nullptr; itr = itr->getRightSibling()) {
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
                        LeafNode<KeyType, DataType> *itr = m_DataHead;
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
                        LeafNode<KeyType, DataType> *itr = result.targetNode;
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

        return results;
    }

    // 范围查询，BETWEEN
    void select(KeyType smallKey, KeyType largeKey, std::vector<DataType>& Result) {
        std::vector<DataType> results;
        if (smallKey <= largeKey) {
            SelectResult<KeyType, DataType> start, end;
            search(smallKey, start);
            search(largeKey, end);
            LeafNode<KeyType, DataType> *itr = start.targetNode;
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
        LeafNode<KeyType, DataType> *itr = m_DataHead;

        while (itr != nullptr) {
            for (int32_t i = 0; i < itr->getKeyNum(); ++i) {
                std::cout << itr->getData(i) << " ";
            }
            std::cout << std::endl;
            itr = itr->getRightSibling();
        }
    }

private:
    void recursive_insert(BaseNode<KeyType> *parentNode, KeyType key, const DataType &data) {
        // 叶子结点，直接插入
        if (parentNode->getType() == LEAF) {
            ((LeafNode<KeyType, DataType> *) parentNode)->insert(key, data);
        } else {
            // 找到子结点
            int32_t keyIndex = parentNode->getKeyIndex(key);
            int32_t childIndex = parentNode->getChildIndex(key, keyIndex); // 孩子结点指针索引
            BaseNode<KeyType> *childNode = ((InternalNode<KeyType> *) parentNode)->getChild(childIndex);

            // 子结点已满，需进行分裂
            if (childNode->getKeyNum() >= MAXNUM_LEAF) {
                childNode->split(parentNode, childIndex);

                // 确定目标子结点
                if (parentNode->getKeyValue(childIndex) <= key) {
                    childNode = ((InternalNode<KeyType> *) parentNode)->getChild(childIndex + 1);
                }
            }
            recursive_insert(childNode, key, data);
        }
    }

    void recursive_remove(BaseNode<KeyType> *parentNode, KeyType key) {
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
            BaseNode<KeyType> *pChildNode = ((InternalNode<KeyType> *) parentNode)->getChild(childIndex); //包含key的子树根节点

            // 包含关键字达到下限值，进行相关操作
            if (pChildNode->getKeyNum() == MINNUM_KEY) {
                // 左兄弟节点
                BaseNode<KeyType> *pLeft = childIndex > 0 ? ((InternalNode<KeyType> *) parentNode)->getChild(childIndex - 1)
                                                          : nullptr;

                // 右兄弟节点
                BaseNode<KeyType> *pRight =
                        childIndex < parentNode->getKeyNum() ? ((InternalNode<KeyType> *) parentNode)->getChild(childIndex + 1)
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

    void printInConcavo(BaseNode<KeyType> *pNode, int32_t count) const {
        if (pNode != nullptr) {
            int32_t i, j;
            for (i = 0; i < pNode->getKeyNum(); ++i) {
                if (pNode->getType() != LEAF) {
                    printInConcavo(((InternalNode<KeyType> *) pNode)->getChild(i), count - 2);
                }
                for (j = count; j >= 0; --j) {
                    std::cout << "-";
                }
                std::cout << pNode->getKeyValue(i) << std::endl;
            }
            if (pNode->getType() != LEAF) {
                printInConcavo(((InternalNode<KeyType> *) pNode)->getChild(i), count - 2);
            }
        }
    }

    bool recursive_search(BaseNode<KeyType> *pNode, KeyType key) const {
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
                    return recursive_search(((InternalNode<KeyType> *) pNode)->getChild(childIndex), key);
                }
            }
        }
    }

    void changeKey(BaseNode<KeyType> *pNode, KeyType oldKey, KeyType newKey) {
        if (pNode != nullptr && pNode->getType() != LEAF) {
            int32_t keyIndex = pNode->getKeyIndex(oldKey);
            if (keyIndex < pNode->getKeyNum() && oldKey == pNode->getKeyValue(keyIndex))  // 找到
            {
                pNode->setKeyValue(keyIndex, newKey);
            } else {  // 继续找
                changeKey(((InternalNode<KeyType> *) pNode)->getChild(keyIndex), oldKey, newKey);
            }
        }
    }

    void search(KeyType key, SelectResult<KeyType, DataType> &result) {
        recursive_search(m_Root, key, result);
    }

    void recursive_search(BaseNode<KeyType> *pNode, KeyType key, SelectResult<KeyType, DataType> &result) {
        int32_t keyIndex = pNode->getKeyIndex(key);
        int32_t childIndex = pNode->getChildIndex(key, keyIndex); // 孩子结点指针索引
        if (pNode->getType() == LEAF) {
            result.keyIndex = keyIndex;
            result.targetNode = (LeafNode<KeyType, DataType> *) pNode;
            return;
        } else {
            return recursive_search(((InternalNode<KeyType> *) pNode)->getChild(childIndex), key, result);
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
                dataValue = ((LeafNode<KeyType, DataType> *) m_Root)->getData(0);
                clear();
                return;
            } else {
                BaseNode<KeyType> *pChild1 = ((InternalNode<KeyType> *) m_Root)->getChild(0);
                BaseNode<KeyType> *pChild2 = ((InternalNode<KeyType> *) m_Root)->getChild(1);
                if (pChild1->getKeyNum() == MINNUM_KEY && pChild2->getKeyNum() == MINNUM_KEY) {
                    pChild1->mergeChild(m_Root, pChild2, 0);
                    delete m_Root;
                    m_Root = pChild1;
                }
            }
        }

        recursive_remove(m_Root, key, dataValue);
    }

    void recursive_remove(BaseNode<KeyType> *parentNode, KeyType key, DataType &dataValue) {
        int32_t keyIndex = parentNode->getKeyIndex(key);
        int32_t childIndex = parentNode->getChildIndex(key, keyIndex); // 孩子结点指针索引

        // 找到目标叶子节点
        if (parentNode->getType() == LEAF) {
            if (key == m_MaxKey && keyIndex > 0) {
                m_MaxKey = parentNode->getKeyValue(keyIndex - 1);
            }

            dataValue = ((LeafNode<KeyType, DataType> *) parentNode)->getData(keyIndex);
            parentNode->removeKey(keyIndex, childIndex);  // 直接删除

            // 如果键值在内部结点中存在，也要相应的替换内部结点
            if (childIndex == 0 && m_Root->getType() != LEAF && parentNode != m_DataHead) {
                changeKey(m_Root, key, parentNode->getKeyValue(0));
            }
        } else {
            // 内结点
            BaseNode<KeyType> *pChildNode = ((InternalNode<KeyType> *) parentNode)->getChild(childIndex); //包含key的子树根节点

            // 包含关键字达到下限值，进行相关操作
            if (pChildNode->getKeyNum() == MINNUM_KEY) {
                BaseNode<KeyType> *pLeft = childIndex > 0 ? ((InternalNode<KeyType> *) parentNode)->getChild(childIndex - 1)
                                                          : nullptr;                       //左兄弟节点
                BaseNode<KeyType> *pRight =
                        childIndex < parentNode->getKeyNum() ? ((InternalNode<KeyType> *) parentNode)->getChild(childIndex + 1)
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
    BaseNode<KeyType> *m_Root;
    LeafNode<KeyType, DataType> *m_DataHead;
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
    /*
    .	分配内存空间
    .	参数：
    .	int32_t size: 列表大小（元素个数）
    .	int32_t type: 0|malloc  1|realloc
    */
    void _AllocSpace(int32_t isize = 0, int32_t type = 0) {
        //新分配的内存起始地址索引
        int32_t start = 0;
        //临时对象数组指针
        ElemType **p = nullptr;
        //1.首次分配内存
        if (type == 0) {
            //this->arr = (ElemType **)malloc(size * sizeof(ElemType *));
            this->arr = new ElemType *[isize];    //参考指针数组，保持 new 与 delete 配对
        }
            //2.重新分配内存
        else if (type == 1) {
            //this->arr = (ElemType **)realloc(this->arr, size * sizeof(ElemType *));
            //将原对象数组元素赋值到新数组中，并删除原数组
            p = this->arr;
            this->arr = new ElemType *[isize];
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
        for (int32_t i = start; i < isize; i++) {
            *(this->arr + i) = nullptr;
        }
    }

    // 判断列表是否满
    bool _IsFull() {
        return this->length == this->size;
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

// 链队列 类模板定义
template<typename ElemType>
class LinkQueue {
    // 链队列
    struct Node {
        ElemType *data;
        Node *next;
    };
private:
    Node *front;    // 队头指针
    Node *rear;    // 队尾指针
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
        // 从front 头结点遍历到链尾
        int32_t i = 0;
        // 条件：尾结点 next == nullptr
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

    bool Empty() {
        return this->top == nullptr;
    }
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
//        EdgeNode *next; // 指向下一条边

        static bool cmp(const EdgeNode &A, const EdgeNode &B){
            return A.adjVex < B.adjVex; // 降序
        }
    };

    // 顶点表结点
    using VertexNode = struct VertexNode {
        int32_t id; // 顶点ID
        int32_t preID;  // 前向节点
        BPlusTree<int32_t, EdgeNode> *pEdgeTable; // 指向边表

        static bool cmp(const VertexNode &A, const VertexNode &B){
            return A.id < B.id; // 降序
        }
    };

public:
    // 边数据，注：供外部初始化边数据使用
    using EdgeData = struct EdgeData {
        int32_t Tail;    // 边（弧）尾
        int32_t Head;    // 边（弧）头
    };

public:
    void addInviteRelationship(int32_t preID, int32_t newID) {
        if (_addVexSet(preID, newID)) {
            EdgeData * edge = new GraphAdjList::EdgeData{ preID, newID };
            ObjArrayList<EdgeData> * edgesList = new ObjArrayList<EdgeData>();
            edgesList->Add(edge);
            _AddEdge(edgesList);
        }
    }


private:
    static const int32_t _MAX_VERTEX_NUM = 10;          // 支持最大顶点数

    BPlusTree<int32_t, VertexNode> vexs;                // 顶点表
    BPlusTree<int32_t, bool> vexs_visited;              // 顶点访问标记数组：0|未访问 1|已访问

    int32_t iVexNum; // 顶点个数
    int32_t iEdgeNum; // 边数

    // 创建顶点集合
    bool _addVexSet(int32_t preID, int32_t newID) {
        // 顶点是否存在
        if (_Locate(preID) != -1) {
            VertexNode vertexNode = { newID, preID, nullptr };

            this->vexs.insert(newID, vertexNode);

            this->iVexNum++;

            return true;
        }

        return false;
    }

    // 创建边
    void _AddEdge(ObjArrayList<EdgeData> *edgesList) {
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
        if (this->vexs.search(vertex)) {
            return vertex;
        }

        // std::cout << std::endl << "顶点[" << vertex << "]不存在。" << std::endl;
        return -1;
    }

    // 插入边
    void _InsertEdge(int32_t tail, int32_t head) {
        // 边结点指针：初始化为 弧尾 指向的第一个边
        auto vertexList = this->vexs.select(tail, EQ);

        if (vertexList.empty()) {
            return;
        }

        VertexNode vertexNode = vertexList[0];

        if (vertexNode.pEdgeTable != nullptr) {
            // 2.1.如果边存在，则跳过，不做插入
            if (vertexNode.pEdgeTable->search(head)) {
                return;
            } else {
                // 2.2.插入边
                EdgeNode newEdge = { head };

                vertexNode.pEdgeTable->insert(tail, newEdge);
            }
        } else {
            // 3.1.边表不存在时，创建边表,插入边
            EdgeNode newEdge = { head };
            BPlusTree<int32_t, EdgeNode> *edges = new BPlusTree<int32_t, EdgeNode>; // 边表

            edges->insert(tail, newEdge);

            vertexNode.pEdgeTable = edges;

            this->vexs.remove(vertexNode.id);
            this->vexs.insert(vertexNode.id, vertexNode);
        }

        // 4.边 计数
        this->iEdgeNum++;
    }

    // 删除边
    void _DeleteEdge(int32_t tail, int32_t head) {
        // 边结点指针：初始化为 弧尾 指向的第一个边
        auto vertexList = this->vexs.select(tail, EQ);

        if (vertexList.empty()) {
            return;
        }

        VertexNode vertexNode = vertexList[0];

        // 初始化 前一边结点的指针
        EdgeNode *q = nullptr;

        // 1.查找边
        if (vertexNode.pEdgeTable == nullptr) {
            return;
        }

        // 若存不在该边，则结束
        if (!vertexNode.pEdgeTable->search(head)) {
            // 2.1.边不存在
            std::cout << std::endl << "边[" << tail << "->" << head << "]不存在。"
                      << std::endl;
            return;
        }

        // 2.2.边存在，删除边
        vertexNode.pEdgeTable->remove(head);
    }

    // 深度优先遍历 递归
    void _DFS_R(int32_t index) {
        // 1.访问顶点，并标记已访问
        auto vertexList = this->vexs.select(index, EQ);

        if (vertexList.empty()) {
            return;
        }

        VertexNode vertexNode = vertexList[0];

        std::cout << vertexNode.id << " ";
        this->vexs_visited.remove(vertexNode.id);
        this->vexs_visited.insert(vertexNode.id, 1);

        // 2.遍历访问其相邻顶点
        BPlusTree<int32_t, EdgeNode> *pEdges = vertexNode.pEdgeTable;
        int32_t adjVex = 0;

        if (pEdges == nullptr) {
            return;
        }

        auto edgeList = pEdges->select(0, BE);

        if (edgeList.empty()) {
            return;
        }

        for (auto edge : edgeList) {
            adjVex = edge.adjVex;
            // 当顶点未被访问过时，可访问
            auto visited = this->vexs_visited.select(adjVex, EQ);
            if (visited.empty()) {
                continue;
            }

            if (visited[0] != true) {
                _DFS_R(adjVex);
            }
        }
    }

//    // 深度优先遍历 非递归
//    void _DFS(int32_t index) {
//        // 1.访问第一个结点，并标记为 已访问
//        auto vertexList = this->vexs.select(index, EQ);
//
//        if (vertexList.empty()) {
//            return;
//        }
//
//        VertexNode vertexNode = vertexList[0];
//
//        std::cout << vertexNode.id << " ";
//
//        this->vexs_visited.remove(vertexNode.id);
//        this->vexs_visited.insert(vertexNode.id, 1);
//
//        // 初始化 边结点 栈
//        LinkStack<std::list<EdgeNode>> *s = new LinkStack<std::list<EdgeNode>>();
//        //初始化边结点 指针
//        BPlusTree<int32_t, EdgeNode> *pEdges = vertexNode.pEdgeTable;
//
//        std::list<EdgeNode> *pEdgeList = nullptr;
//
//        //2.寻找下一个（未访问的）邻接结点
//        while (!s->Empty() || pEdges != nullptr) {
//            //2.1.未访问过，则访问 （纵向遍历）
//
//            auto tempSet = pEdges->select(0, BE);
//
//            if (pEdgeList->empty()) {
//                return;
//            }
//
//            pEdgeList = new std::list<EdgeNode>();
//            pEdgeList->assign(tempSet.begin(), tempSet.end());
//
//            auto visited = this->vexs_visited.select(pEdgeList->front().adjVex, EQ);
//
//            if (visited.empty()) {
//                continue;
//            }
//
//            if (visited[0] != true) {
//                //访问结点，标记为访问，并将其入栈
//                auto vertexList = this->vexs.select(pEdgeList->front().adjVex, EQ);
//
//                if (vertexList.empty()) {
//                    continue;
//                }
//
//                VertexNode vertexNode = vertexList[0];
//
//                std::cout << vertexNode.id << " " << std::flush;
//
//                this->vexs_visited.remove(pEdgeList->front().adjVex);
//                this->vexs_visited.insert(pEdgeList->front().adjVex, 1);
//
//                s->Push(pEdgeList);
//
//                pEdges = vertexNode.pEdgeTable;
//            } else {
//                pEdgeList->pop_front();
//                if (pEdgeList->empty()) {
//                    delete pEdgeList;
//                }
//            }
//
//            if (pEdges == nullptr) {
//                pEdgeList = s->Pop();
//            }
//        }
//        //释放 栈
//        delete s;
//    }

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
        VertexNode vertexNode = { 0, -1, nullptr };
        this->vexs.insert(0, vertexNode);
    }

//    //插入边
//    void InsertEdge(const EdgeData &edgeData) {
//        // 初始化 Tail Head 顶点下标索引
//        int32_t tail = 0, head = 0;
//        tail = _Locate(edgeData.Tail);
//        head = _Locate(edgeData.Head);
//        // 根据图类型，插入边
//        _InsertEdge(tail, head);
//    }
//
//    // 删除边
//    void DeleteEdge(const EdgeData &edgeData) {
//        // 初始化 Tail Head 顶点下标索引
//        int32_t tail = 0, head = 0;
//        tail = _Locate(edgeData.Tail);
//        head = _Locate(edgeData.Head);
//
//        // 根据图类型，删除边
//        _DeleteEdge(tail, head);
//    }

    // 显示 图
    void Display() {
        // 初始化边表结点指针
        std::cout << std::endl << "邻接表：" << std::endl;

        // 遍历顶点表
        auto vertexList = this->vexs.select(0, BE);

        if (vertexList.empty()) {
            return;
        }

        for (auto vex : vertexList) {
            // 输出顶点
            std::cout << "[" << vex.id << "]" << vex.id << " ";

            // 遍历输出边顶点
            auto pEdges = vex.pEdgeTable;

            if (pEdges == nullptr) {
                std::cout << std::endl;
                continue;
            }

            // 遍历顶点表
            auto edgeList = pEdges->select(0, BE);

            if (edgeList.empty()) {
                continue;
            }

            for (auto edge : edgeList) {
                std::cout << "[" << edge.adjVex << "] ";
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

        auto vertexList = this->vexs.select(0, BE);

        if (vertexList.empty()) {
            return;
        }

        // 2.初始化顶点访问数组
        for (auto vex : vertexList) {
            this->vexs_visited.insert(vex.id, 0);
        }

        // 3.深度优先遍历 递归
        std::cout << "深度优先遍历（递归）：（从顶点" << vertex << "开始）" << std::endl;
        _DFS_R(index);
    }

//    // 从指定顶点开始，深度优先 非递归 遍历
//    void Display_DFS(int32_t vertex) {
//        // 1.判断顶点是否存在
//        int32_t index = _Locate(vertex);
//        if (index == -1)
//            return;
//
//        auto vertexList = this->vexs.select(0, BE);
//
//        if (vertexList.empty()) {
//            return;
//        }
//
//        // 2.初始化顶点访问数组
//        for (auto vex : vertexList) {
//            this->vexs_visited.remove(vex.id);
//            this->vexs_visited.insert(vex.id, 0);
//        }
//
//        // 3.深度优先遍历 递归
//        std::cout << "深度优先遍历（非递归）：（从顶点" << vertex << "开始）" << std::endl;
//        _DFS(index);
//    }

    // 从指定顶点开始，广度优先遍历
    void Display_BFS(int32_t vertex) {
        // 1.判断顶点是否存在
        int32_t index = _Locate(vertex);
        if (index == -1)
            return;

        {
            auto vertexList = this->vexs.select(0, BE);

            if (vertexList.empty()) {
                return;
            }

            // 2.初始化顶点访问数组
            for (auto vex : vertexList) {
                this->vexs_visited.remove(vex.id);
                this->vexs_visited.insert(vex.id, 0);
            }
        }

        // 3.广度优先遍历
        std::cout << "广度优先遍历：（从顶点" << vertex << "开始）" << std::endl;

        // 3.1.初始化队列
        LinkQueue<int32_t> * vexQ = new LinkQueue<int32_t>();

        // 3.2.访问开始顶点，并标记访问、入队
        auto vertexList = this->vexs.select(index, EQ);

        if (vertexList.empty()) {
            return;
        }

        VertexNode vertexNode = vertexList[0];

        std::cout << vertexNode.id << " " << std::flush;

        this->vexs_visited.remove(index);
        this->vexs_visited.insert(index, 1);
        vexQ->EnQueue(new int32_t(index));

        // 3.3.出队，并遍历邻接顶点（下一层次），访问后入队
        BPlusTree<int32_t, EdgeNode> *pEdges = nullptr;
        int32_t adjVex = 0;
        while (vexQ->GetHead() != nullptr) {
            {
                index = *vexQ->DeQueue();
                auto vertexList = this->vexs.select(index, EQ);

                if (vertexList.empty()) {
                    return;
                }

                VertexNode vertexNode = vertexList[0];

                pEdges = vertexNode.pEdgeTable;

                if (pEdges == nullptr) {
                    continue;
                }
            }

            // 遍历邻接顶点
            auto edgeList = pEdges->select(0, BE);

            if (edgeList.empty()) {
                return;
            }

            for (auto edge : edgeList) {
                // 未访问过的邻接顶点
                adjVex = edge.adjVex;
                auto visited = this->vexs_visited.select(adjVex, EQ);

                if (visited.empty()) {
                    continue;
                }

                if (visited[0] != true) {
                    // 访问顶点，并标记访问、入队
                    auto vertexSetTemp = this->vexs.select(adjVex, EQ);

                    if (vertexSetTemp.empty()) {
                        return;
                    }

                    VertexNode vertexNode = vertexSetTemp[0];

                    std::cout << vertexNode.id << " " << std::flush;

                    this->vexs_visited.remove(adjVex);
                    this->vexs_visited.insert(adjVex, 1);

                    vexQ->EnQueue(new int32_t(adjVex));
                }
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

    // 测试1：无向图
    std::cout << std::endl << "图初始化：" << std::endl;
    GraphAdjList *dg = new GraphAdjList();
    dg->Init();

    dg->addInviteRelationship(0, 1);
    dg->addInviteRelationship(1, 2);
    dg->addInviteRelationship(0, 3);
    dg->addInviteRelationship(3, 4);
    dg->addInviteRelationship(4, 5);
    dg->addInviteRelationship(4, 6);
    dg->addInviteRelationship(4, 7);
    dg->addInviteRelationship(2, 8);

    dg->Display();

    int32_t * id0 = new int32_t (0);
    int32_t * id1 = new int32_t (4);

    // 1.1.深度优先遍历
    std::cout << std::endl << "图深度优先遍历序列：（递归）" << std::endl;
    dg->Display_DFS_R(*id0);

    // 1.2.广度优先遍历
    std::cout << std::endl << std::endl << "图广度优先遍历序列：" << std::endl;
    dg->Display_BFS(*id0);

    // 1.3.插入新边
    std::cout << std::endl << std::endl << "新边：" << std::endl;
    dg->addInviteRelationship(7, 9);
    dg->Display();

    return 0;
}
