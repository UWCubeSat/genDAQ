///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Pre-Processor & Forward Declarations
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "Arduino.h"

#define divCeiling(x, y) (!!x + ((x - !!x) / y))

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Memory Pool
///////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T> class MemPool {
  private:
    T *memArray;
    int16_t bytes;
    int16_t elements;
    int16_t currentIndex;

  public:
    MemPool(int16_t bytes) {
      this->bytes = bytes;
      this->elements = divCeiling(bytes, sizeof(T));
      memArray = new T[elements]();
    }

    T *request() {
      T *targElement = nullptr;
      if (currentIndex > 0) {
        targElement = memArray[]
      }
    }



  private:
};


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> LinkedList
///////////////////////////////////////////////////////////////////////////////////////////////////

const int16_t LINKEDLIST_MAX_SIZE = 256;

// @brief: Represents an indicie in the list.
template<typename T> struct Node {         
  Node<T> *prevNode = nullptr;
  Node<T> *nextNode = nullptr;
  T data = T();

  Node(Node<T> *prevNode, Node<T> *nextNode, T value):prevNode{prevNode},
    nextNode{nextNode},data{value}{} 

 operator const T() const { return data; }
  Node<T> &operator=(T newData) {
    data = newData;
    return *this;
  }
};

// @brief: Optimized for: adding/inserting/removing elements, reading elements from
//         front/end of list & dynamic length. Not optimized for: frequent traversal,
//         & accessing specific indicies.
template<typename T> class LinkedList {
  private:
    int16_t currentSize = 0;
    int16_t recentIndex = 0;
    Node<T> *head = nullptr;
    Node<T> *tail = nullptr;
    Node<T> *recentNode = nullptr; //For automatic iteration.
    int16_t maxSize = 0;

    Node<T> *currentNode = head;
    int16_t currentIndex = 0;

  public:
    // #constructor
    // @brief: Assigns max size to associated setting.
    LinkedList() {
      maxSize = LINKEDLIST_MAX_SIZE;
    }

    LinkedList(int8_t test) {
      maxSize = LINKEDLIST_MAX_SIZE;
    }

    // #constructor
    // @brief: Copy constructor
    // @param: otherList -> list to copy from.
    //         maxSize   -> max nodes in list.
    LinkedList(LinkedList<T> &otherList) : LinkedList() {
      for (int16_t i = 0; i < otherList.size() && i < LINKEDLIST_MAX_SIZE; i++)  {
        createNode(otherList.read(i), currentSize);
      }
    }

    // @breif: Insert value at index (non-overriding)
    // @param: value -> element to add. 
    //         index -> index of new element
    // @note: If index is equal to size of list, element will be added to end.
    bool add(T value, int16_t index) {
      if (index > currentSize || index < 0) { index = currentSize; }
      if (currentSize + 1 > maxSize) { return false; }

      createNode(value, index);
      return true;
    }

    // #overload
    // @breif: Adds a value onto the end of the list.
    // @param: value -> element to add.
    void add(T value) { add(value, currentSize); }

    // @brief: Removes & returns value in the list.
    // @param: index -> index to remove/return.
    T remove(int16_t index) {
      if (index >= currentSize || index < 0) { return T(); }
      if (currentSize == 0, PROG_INVALID_REQUEST) { return T(); }

      T indexValue = read(index);
      removeNode(index);
      return indexValue;
    }

    //#overload
    // @brief: Returns object in beggining of list.
    T remove() {
      return remove(0);
    }

    // @breif: Remove all values equal to param from list
    // @param: value -> value to be remove.
    // @return: The number of elements removed.
    int16_t removeAll(T value) {
      if (currentSize == 0) { return 0; }

      resetNext();
      int16_t removeCount = 0;
      int16_t result = 0;
      int16_t timeoutCount = 0;
      while(result != -1) {
        result = nextNode(value);
        if (result != -1) {
          removeCount++;
          removeNode(index);
        }
        if (timeoutCount > LINKEDLIST_MAX_SIZE) { break; }
        timeoutCount++;
      }
      return removeCount;
    }

    // @brief: Returns a value from the list.
    // @param: index -> index to get value from.
    T get(int16_t index) {
      if (index >= currentSize || index < 0) { index = 0; }
      if (currentSize == 0) { return T(); }

      return read(index); 
    }

    // @brief: Allows access & assignment of list in the same fassion as
    //         an array -> myList[3] = "hello"; String myStr = myList[3];
    Node<T> &operator[](int16_t index) {
      return *getNode(index);
    } 

    // @brief: Overwrites the value at an index.
    // @param: index -> index that  will be written to. 
    //         value -> element to be written.
    // @WARN: Do not pass indecies that are >= the size of the list - you cannot
    //        add elements to the end of the list using this method.
    T set(int16_t index, T value) {
      if (index >= currentSize || index < 0) { return T(); }
      if (currentSize == 0) { return T(); }

      T indexValue = read(index);
      write(value, index);
      return indexValue;
    }

    // @brief: Inserts a copy of annother list into current list.
    // @param: otherList -> list to append. 
    //         index     -> index to insert copy.
    void append(LinkedList<T> *otherList, int16_t index) {
      if (index > currentSize || index < 0) { index = currentSize; }

      if (index == currentSize) {
        for (int16_t i = 0; i < otherList->currentSize && i < LINKEDLIST_MAX_SIZE; i++) {
          createNode(otherList->read(i), currentSize);
        }
      } else {
        for (int16_t i = otherList->currentSize - 1; i >= 0; i--) {
          createNode(otherList->read(i), index);
        }
      }
    }

    // @brief: Overload for easily appending list to end of list.
    // @param: otherList -> list to append.
    void append(LinkedList<T> *otherList) { append(otherList, currentSize); }

    // @brief: Returns the size of the list (non-zero based)
    int16_t size() { return currentSize; }

    // @brief: Returns true if the list is empty.
    bool isEmpty() { return (currentSize == 0); }

    // @brief: Returns true if a value is contained in list.
    // @param: value -> value to look for in list.
    bool contains(T value) {
      if (currentSize == 0) { return false; }

      for (int16_t i = 0; i < currentSize && i < LINKEDLIST_MAX_SIZE; i++) {
        if (read(i) == value) {
          return true;
        }
      }
      return false;
    }

    // @brief: Gets index of first occurance of value.
    // @param: value -> value to search for.
    // @note: If element is not found method will return (-1).
    int16_t indexOf(T value) {
      if (currentSize == 0) { return -1; }

      resetNext();
      return nextNode(value);
    }

    // @brief: Removes all elements from the list.
    void clear() {
      if (currentSize == 0) { return; }

      int16_t timeoutCount = 0;
      while(head != NULL) {
        Node<T> *nextNode = head->nextNode;
        delete head;
        head = nextNode;
        if (timeoutCount > LINKEDLIST_MAX_SIZE) { break; }
        timeoutCount++;
      }
      currentSize = 0;
    }

    // @brief: Gets a pointer to a specific node in the linked list
    // @NOTE: Calling this method does not remove the node from the
    //        list so DO NOT delete it or change it's link attributes.
    Node<T> *getPtr(int16_t index) {
      return getNode(index);
    }

    // #destructor
    ~LinkedList() {
      clear();
    } 

  private:
    // @breif: creates new node with "value" in linked list at "index".
    void createNode(T value, int16_t index) {
      if (index == 0) {
        if (currentSize == 0) {
          Node<T> *firstNode = new Node<T>(nullptr, nullptr, value);
          head = firstNode;
          tail = firstNode;
        } else {
          Node<T> *newNode = new Node<T>(nullptr, head, value);
          head->prevNode = newNode;
          head = newNode;
        }
        currentSize++;
        return;
      } else if (index == currentSize) {
        Node<T> *newNode = new Node<T>(tail, nullptr, value);
        tail->nextNode = newNode;
        tail = newNode;
        currentSize++;
        return;
      }

      Node<T> *indexNode = getNode(index);
      Node<T> *prevNode = indexNode->prevNode;
      Node<T> *nextNode = indexNode;

      Node<T> *newNode = new Node<T>(prevNode, nextNode, value);
      prevNode->nextNode = newNode;
      nextNode->prevNode = newNode;
      currentSize++;
    }

    // @breif: Removes node from linked list at "index".
    void removeNode(int16_t index) {
      Node<T> *node = getNode(index);
      if (node->nextNode != NULL) {
        recentNode = node->nextNode;
        recentIndex++;
      } else if (node->prevNode != NULL) {
        recentNode = node->prevNode;
        recentIndex--;
      } else {
        recentNode = nullptr;
      }
      if (currentSize > 1) {
        if (index == 0) {
          head = node->nextNode;
          node->nextNode->prevNode = nullptr;
          delete node;
          if (currentSize == 2) {
            tail = head;
          }
        } else if (index == currentSize - 1) {
          tail = node->prevNode;
          node->prevNode->nextNode = nullptr;
          delete node;
          if (currentSize == 2) {
            head = tail;
          }
        } else {
          node->prevNode->nextNode = node->nextNode;
          node->nextNode->prevNode = node->prevNode;
          delete node;
        }
      } else {
        delete node;
        head = nullptr;
        tail = nullptr;
      }
      currentSize--;
    }

    // @breif: Returns data contained at node at "index".
    T read(int16_t index) {
      Node<T> *indexNode = getNode(index);
      return indexNode->data;
    } 

    // @breif: Stores the value - "data" to node at "index".
    void write(T data, int16_t index) {
      Node<T> *indexNode = getNode(index);
      indexNode->data = data;
    }

    // @brief: Gets a reference to node at specific index,
    //         using closest pointer (head/tail/recent).
    Node<T> *getNode(int16_t index) {
      Node<T> *node = nullptr;
      //Is index closer to last changed node then the beggining/end of list?
      if (abs(recentIndex - index) <= index && abs(recentIndex - index) <= (currentSize - 1 - index) 
        && recentNode != NULL) {
        node = recentNode;
        if (recentIndex - index < 0) { 
          for (int16_t i = recentIndex; i < index && i < LINKEDLIST_MAX_SIZE; i++) {
            node = node->nextNode;
          }
        } else if (recentIndex - index > 0){
          for (int16_t i = recentIndex; i > index && i >= 0; i--) {
            node = node->prevNode;
          }
        }
      } else if (index > (currentSize - 1) / 2) {
        node = tail;
        for (int16_t i = currentSize - 1; i > index && i >= 0; i--) {
          node = node->prevNode;
        }
      } else {
        node = head;
        for (int16_t i = 0; i < index && i < LINKEDLIST_MAX_SIZE; i++) {
          node = node->nextNode;
        }
      }

      recentNode = node;
      recentIndex = index;
      return node;
    }

    // @breif: Returns index of next node with "value" (used for "removeAll" method)
    // @note: Using this function for searching through list is much more efficient.
    // @note: Method will return -1 when it reaches the end of the list.
    int16_t nextNode(T value) {
      if (currentNode == NULL) { return -1; } //Not an error.

      int16_t timeoutCount = 0;
      while(currentNode != NULL) {
        if (currentNode->data == value) {
          currentIndex++;
          currentNode = currentNode->nextNode;
          return currentIndex - 1;
        } else {
          currentNode = currentNode->nextNode;
          currentIndex++;
        }
        if (DENY(timeoutCount > LINKEDLIST_MAX_SIZE, PROG_TIMEOUT)) { break; }
        timeoutCount++;
      }
      return -1;
    }

    // @breif: Resets the iterator such that it starts at beggining again (see above)
    void resetNext() {
      currentNode = head;
      currentIndex = 0;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Circular Buffer
///////////////////////////////////////////////////////////////////////////////////////////////////
