///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Pre-Processor & Forward Declarations
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Arduino.h"

#define divCeiling(x, y) (!!x + ((x - !!x) / y))

template<typename T> class MemoryPool;
template<typename T> class Node;
template<typename T> class LinkedList;

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Memory Pool
///////////////////////////////////////////////////////////////////////////////////////////////////

// Max number of ELEMENTS that can be in a memeory pool, regardless of size.
const int16_t MEMORY_POOL_MAX_SIZE = 512; 

template<typename T> class MemoryPool {
  private:
    T **memArray;
    int16_t totalSize;
    int16_t currentSize;

  public:
    // #Constructor
    // @brief: Creates a memory pool.
    // @param: elements -> Specifies the number of elements (specified
    //                     by template type) that the pool will contain.
    MemoryPool(int16_t elements) {
      // Ensure requested bytes don't exceed max size.
      if (elements > MEMORY_POOL_MAX_SIZE) { elements = MEMORY_POOL_MAX_SIZE; }
      this->totalSize = elements;
      currentSize = elements;
      // Create array of pointers.
      memArray = new T*[elements]();
      // Allocate objects & assign them to each pointer.
      for (int16_t i = 0; i < elements && MEMORY_POOL_MAX_SIZE; i++) {
        memArray[i] = new T();
      }
    }

    // @brief: Returns ptr to next available element from the pool.
    // @NOTE: Do not delete this element, pass it back to the
    //        pool through the method "returnElement".
    T *borrowElement() { 
      T *targElement = nullptr;
      // Are there any un-allocated elements in the array?
      if (currentSize > 0) {
        targElement = memArray[currentSize - 1];
        memArray[currentSize - 1] = nullptr;
        currentSize--;
      }
      return targElement; 
    } 

    // @brief: Returns the passed pointer's element to the pool.
    // @param: targElement -> ADDRESS of Pointer to the element you with to return to the pool. 
    //         Please note this ptr will be set to null after passed to this method.
    //         Example call: (with, *myPtr) -> returnElement(&myPtr);
    // @return: True if successful, false otherwise -> element null, or pool full.
    bool returnElement(T **targElement) {
      if (*targElement == nullptr) { return false; }
      if (currentSize == totalSize) { return false; }
      // Element and pool size = valid so add returned element.
      memArray[currentSize] = *targElement;
      currentSize++;
      *targElement = nullptr;
      return true;
    }

    // @brief: Removes unused memory from the array.
    // @param: elementCount -> number of elements to prune from pool (if possible).
    // @return: An integer representing the number of bytes actually removed.
    int16_t deallocate(int16_t elementCount) {
      if (elementCount < 0) { return 0; }
      if (elementCount > currentSize) { elementCount = currentSize; }
      // Create new, smaller array and copy elements over.
      T **newArray = new T*[totalSize - elementCount];
      for (int16_t i = elementCount; i < totalSize && i < MEMORY_POOL_MAX_SIZE; i++) {
         newArray[i - elementCount] = memArray[i];
      }
      // Now delete "pruned" elements from previous array.
      for(int16_t i = 0; i < elementCount && MEMORY_POOL_MAX_SIZE; i++) {
        if (memArray[i] != NULL) {
          delete memArray[i];
        }
      }
      // Config variables.
      delete[] memArray;
      memArray = newArray;
      totalSize -= elementCount;
      currentSize -= elementCount;
      return sizeof(T) * elementCount;
    }

    // #Overload
    // @brief: Returns as many elements as possible from pool.
    int16_t deallocate() {
      return deallocate(currentSize);
    }

    // @brief: Expands the size of the pool.
    // @param: elementCount -> Number of new elements to allocate in the pool.
    // @return: An integer that denotes the number of bytes added to the pool.
    // @NOTE: This is a very expensive opperation! So if possible avoid 
    //        expanding the memory pool.
    int16_t allocate(int16_t elementCount) {
      if (elementCount < 0) { return 0; }
      if (totalSize + elementCount > MEMORY_POOL_MAX_SIZE) { 
        elementCount = MEMORY_POOL_MAX_SIZE - totalSize;
      }
      // Create new pool and copy elements over.
      T **newArray = new T*[totalSize + elementCount]();
      for (int16_t i = 0; i < totalSize && i < MEMORY_POOL_MAX_SIZE; i++) {
        newArray[i + elementCount] = memArray[i];
      }
      // Create new elements in new mem array.
      for (int16_t i = 0; i < elementCount && i < MEMORY_POOL_MAX_SIZE; i++) {
        newArray[i] = new T();
      }
      // Delete old pool array & set pointer to new one.
      delete[] memArray;
      memArray = newArray;
      totalSize += elementCount;
      currentSize += elementCount;
      return sizeof(T) * elementCount;
    }

    // @brief: Returns the number of not-borrowed elements in 
    //         the pool.
    int16_t remainingElements() {
      return currentSize;
    }

    // @brief: Returns the total number of elements that the
    //         pool can hold.
    int16_t totalElements() {
      return totalSize;
    }

    // @brief: Returns an estimate for the number of bytes taken
    //         up by the memory pool.
    int16_t totalBytes() {
      return sizeof(T) * totalSize;
    }

    // @brief: Returns true if there are un-borrowed elements 
    //         remaining in the pool & false otherwise.
    bool hasRemaining() {
      return (currentSize > 0);
    }


    // #Destructor
    // @breif: Destroys all elements in the pool.
    // @NOTE: Any object not returned to the pool before it is deleted will
    //        ~NOT~ be deleted by deleting the pool.
    ~MemoryPool() {
      for (int16_t i = 0; i < totalSize && i < MEMORY_POOL_MAX_SIZE; i++) {
        if (memArray[i] != NULL) {
          delete memArray[i];
        }
      }
      delete[] memArray;
    }
};


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> LinkedList
///////////////////////////////////////////////////////////////////////////////////////////////////

// Max "nodes" (values) in linked list.
const int16_t LINKEDLIST_MAX_SIZE = 256;

const int16_t LINKEDLIST_DYNAMIC_SIZING_MULTIPLIER = 50;

// @brief: Represents a single index in the linked list.
template<typename T> struct Node {  
  friend class LinkedList<T>;
  public:
    Node() {}
    T data = T();

    operator const T() const { return data; }
    Node<T> &operator=(T newData) {
      data = newData;
      return *this;
    }

  private:
    Node<T> *prevNode = nullptr;
    Node<T> *nextNode = nullptr;
    
    void init(Node<T> *prevNode, Node<T> *nextNode, T value) {
      this->prevNode = prevNode;
      this->nextNode = nextNode;
      data = value;
    }

    void reset() {
      prevNode = nullptr;
      nextNode = nullptr;
      data = T();
    }
};

// @brief: Doubly linked list with dynamic memory pool & auto iteration.
// @NOTE: Keep in mind:
//   Strengths -> Moving, shifting & inserting values into list.
//             -> Iterating through list or getting first/last value - O(n) / O(1)
//             -> Variable size.
//
//  Weaknesses -> Storing many individual elements.
//             -> High SRAM usage - dynamically allocated nodes = stored on heap...
template<typename T> class LinkedList {
  private:
    int16_t currentSize = 0; // Current number of nodes in list.
    int16_t recentIndex = 0; // Used for iterating through list.
    Node<T> *head = nullptr; // Front/beginning of list.
    Node<T> *tail = nullptr; // Back/end of list.
    Node<T> *recentNode = nullptr; //For automatic iteration.

    MemoryPool<Node<T>> *pool; 
    bool autoSizing = false; // If true -> List automatically requests more capacity from pool when full.
    int16_t deltaSize = 0; // Change in size from original requested capacity.

  public:
    // #constructor
    // @brief: Assigns max size to associated setting.
    // @param numIndicies -> initial capacity of linked list.
    //         autoSizing -> If true capacity will auto increase when filled.
    // @NOTE: autoSizing will never decrease capacity, only increase it...
    LinkedList(int16_t numIndicies, bool autoSizing) : pool(new MemoryPool<Node<T>>(numIndicies)) {
      if (numIndicies > LINKEDLIST_MAX_SIZE) { numIndicies = LINKEDLIST_MAX_SIZE; }
      this->autoSizing = autoSizing;
    }

    // #constructor
    // @brief: Copy constructor
    // @param:  otherList -> list to copy from.
    //         autoSizing -> see above.
    LinkedList(LinkedList<T> &otherList, bool autoSizing) : LinkedList(otherList.pool->totalSize, 
               autoSizing) {
      for (int16_t i = 0; i < otherList.size() && i < LINKEDLIST_MAX_SIZE; i++)  {
        createNode(otherList.read(i), currentSize);
      }
    }

    // @breif: Insert value at index (non-overriding)
    // @param: value -> element to add. 
    //         index -> index of new element
    // @note: If index is equal to size of list, element will be added to end.
    bool add(T value, int16_t index) {      
      if ((currentSize + 1 > LINKEDLIST_MAX_SIZE) || 
        (!autoSizing && currentSize >= pool->hasRemaining())) { 
        return false; 
      } else if (index > currentSize || index < 0) { 
        index = currentSize; 
      }

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
      if (currentSize == 0) {
        return T();
      } else if (index < 0) {
        index = 0;
      } else if (index >= currentSize) {
        index = currentSize - 1;
      }

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
      int16_t removeCount = 0;
      for (int i = currentSize - 1; i >= 0; i--) {
        if (read(i) == value) {
          removeCount++;
          removeNode(i);
        }
      }
      return removeCount;
    }

    // @brief: Returns a value from the list.
    // @param: index -> index to get value from.
    T get(int16_t index) {
      if (currentSize == 0) {
        return T();
      } else if (index < 0) {
        index = 0;
      } else if (index > currentSize) {
        index = currentSize - 1;
      }

      return read(index); 
    }

    // @brief: Overwrites the value at an index.
    // @param: index -> index that  will be written to. 
    //         value -> element to be written.
    // @WARN: Do not pass indecies that are >= the size of the list - you cannot
    //        add elements to the end of the list using this method.
    T set(int16_t index, T value) {
      if (currentSize == 0) {
        return T();
      } else if (index < 0) {
        index = 0;
      } else if (index > currentSize) {
        index = currentSize - 1;
      }

      T indexValue = read(index);
      write(value, index);
      return indexValue;
    }

    // @brief: Inserts a copy of annother list into current list.
    // @param: otherList -> list to append. 
    //         index     -> index to insert copy.
    // @Return: True if append was successful (occured), false otherwise.
    bool append(LinkedList<T> *otherList, int16_t index) {
      int16_t appendCount = otherList->currentSize;
      // Check for exceptions & adjust quantities to "correct" for them...
      if (currentSize + otherList->currentSize > LINKEDLIST_MAX_SIZE) {
        appendCount = LINKEDLIST_MAX_SIZE - currentSize;
      } else if(!autoSizing && otherList->currentSize + currentSize > pool->remainingElements()) {
        appendCount = pool->remainingElements() - currentSize;
      } else if (index < 0) {
        index = 0;
      } else if (index > currentSize) {
        index = currentSize - 1;
      }
      // Add in the other list by copying elements over one at a time.
      if (index == currentSize) {
        for (int16_t i = 0; i < appendCount && i < LINKEDLIST_MAX_SIZE; i++) {
          createNode(otherList->read(i), currentSize);
        }
      } else {
        for (int16_t i = appendCount - 1; i >= 0; i--) {
          createNode(otherList->read(i), index);
        }
      }
      recentNode = head;
      recentIndex = 0;
      return true;
    }

    // @brief: Allows access & assignment of list in the same fassion as
    //         an array -> myList[3] = "hello"; String myStr = myList[3];
    Node<T> &operator[](int16_t index) {
      return *getNode(index);
    } 

    // @brief: Overload for easily appending list to end of list.
    // @param: otherList -> list to append.
    void append(LinkedList<T> *otherList) { append(otherList, currentSize); }

    // @brief: Returns the size of the list (non-zero based)
    int16_t size() { return currentSize; }

    // @brief: Returns true if the list is empty.
    bool isEmpty() { return (currentSize == 0); }

    // @brief: Returns the number of elements (filled/not filled) in the linked list.
    int16_t capacity() { return pool->totalElements(); }

    // @brief: Returns number of nodes that remain unused/unallocated in list.
    int16_t remainingCapacity() { return (pool->totalElements() - currentSize); }

    // @brief: Returns the total bytes taken up by the linked list.
    int16_t bytes() { return pool->totalBytes(); }

    // @brief: Reduces capacity of linked list.
    // @param: nodes -> Number of nodes to attempt to prune.
    // @return: The number of bytes pruned from the pool.
    // @NOTE: You can do this even if auto sizing is turned off!
    int16_t prune(int16_t nodes) { return pool->deallocate(nodes); }

    // @brief: Increases capacity of linked list.
    // @param: nodes -> Number of nodes to add to capacity.
    // @return: Number of bytes allocated by adding nodes.
    int16_t expand(int16_t nodes) { return pool->allocate(nodes); }

    // @brief: Returns true if a value is contained in list.
    // @param: value -> value to look for in list.
    bool contains(T value) { return indexOf(value) != -1; }

    // @brief: Gets index of first occurance of value.
    // @param: value -> value to search for.
    // @note: If element is not found method will return (-1).
    int16_t indexOf(T value) {
      for (int i = 0; i < currentSize && i < LINKEDLIST_MAX_SIZE; i++) {
        if (read(i) == value) {
          return i;
        }
      }
      return -1;
    }

    // @brief: Returns how many instances of a specific value there are
    //         in the list.
    // @param: The value you want to query.
    int16_t count(T value) {
      int16_t currentCount = 0;
      for (int i = 0; i < currentSize; i++) {
        if (read(i) == value) {
          currentCount++;
        }
      }
      return currentCount;
    }

    // @brief: Removes all elements from the list.
    // @NOTE: This method does not actually delete any
    //        of the nodes. It just "resets" the list.
    void clear() {
      if (currentSize == 0) { return; }

      int16_t timeoutCount = 0;
      while(head != NULL) {
        Node<T> *nextNode = head->nextNode;
        deleteNode(head);
        head = nextNode;
        if (timeoutCount > LINKEDLIST_MAX_SIZE) { break; }
        timeoutCount++;
      }
      recentNode = nullptr;
      recentIndex = 0;
      currentSize = 0;
    }

    // #destructor
    ~LinkedList() {
      // Clear list so all nodes are returned to pool.
      clear();
      // Then delete the actual pool...
      delete pool;
    } 

  private:
    Node<T> *newNode() {
      Node<T> *targNode = nullptr;
      // Is mem pool empty?
      if (pool->hasRemaining()) {
        targNode = pool->borrowElement();
      } else {
        // We need more elements, so can we increase size?
        if (autoSizing) {
          int16_t multiplierResult = currentSize * LINKEDLIST_DYNAMIC_SIZING_MULTIPLIER;
          pool->allocate(divCeiling(multiplierResult, 100));
          targNode = pool->borrowElement();
          deltaSize++;  
        } else {
          // TO DO -> Log Error?
        }
      }
      return targNode;
    }

    void deleteNode(Node<T> *targNode) { 
      targNode->reset();
      pool->returnElement(&targNode);
      // TO DO -> Reduce size of pool?
    }

    // @breif: creates new node with "value" in linked list at "index".
    void createNode(T value, int16_t index) {
      // Is at start of list?
      if (index == 0) { 
        if (currentSize == 0) {
          Node<T> *targNode = newNode();
          targNode->init(nullptr, nullptr, value);
          head = targNode;
          tail = targNode;
        } else {
          Node<T> *targNode = newNode();
          targNode->init(nullptr, head, value);
          head->prevNode = targNode;
          head = targNode;
        }
        currentSize++;
        return;
      // Is it at the end of the list?
      } else if (index == currentSize) {
        Node<T> *targNode = newNode();
        targNode->init(tail, nullptr, value);
        tail->nextNode = targNode;
        tail = targNode;
        currentSize++;
        return;
      }
      // Not at start or end -> must be in ~middle~ of list.
      Node<T> *indexNode = getNode(index);
      Node<T> *prevNode = indexNode->prevNode;
      Node<T> *nextNode = indexNode;

      Node<T> *targNode = newNode();
      targNode->init(prevNode, nextNode, value);
      prevNode->nextNode = targNode;
      nextNode->prevNode = targNode;
      currentSize++;
    }

    // @breif: Removes node from linked list at "index".
    void removeNode(int16_t index) {
      Node<T> *targNode = getNode(index);
      if (targNode->nextNode != NULL) {
        recentNode = targNode->nextNode;
        recentIndex++;
      } else if (targNode->prevNode != NULL) {
        recentNode = targNode->prevNode;
        recentIndex--;
      } else {
        recentNode = nullptr;
      }
      if (currentSize > 1) {
        if (index == 0) {
          head = targNode->nextNode;
          targNode->nextNode->prevNode = nullptr;
          deleteNode(targNode);
          if (currentSize == 2) {
            tail = head;
          }
        } else if (index == currentSize - 1) {
          tail = targNode->prevNode;
          targNode->prevNode->nextNode = nullptr;
          deleteNode(targNode);
          if (currentSize == 2) {
            head = tail;
          }
        } else {
          targNode->prevNode->nextNode = targNode->nextNode;
          targNode->nextNode->prevNode = targNode->prevNode;
          deleteNode(targNode);
        }
      } else {
        deleteNode(targNode);
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
      Node<T> *targNode = nullptr;
      //Is index closer to last changed node then the beggining/end of list?
      if (abs(recentIndex - index) <= index && abs(recentIndex - index) <= (currentSize - 1 - index) 
        && recentNode != NULL) {
        targNode = recentNode;
        if (recentIndex - index < 0) { 
          for (int16_t i = recentIndex; i < index && i < LINKEDLIST_MAX_SIZE; i++) {
            targNode = targNode->nextNode;
          }
        } else if (recentIndex - index > 0){
          for (int16_t i = recentIndex; i > index && i >= 0; i--) {
            targNode = targNode->prevNode;
          }
        }
      } else if (index > (currentSize - 1) / 2) {
        targNode = tail;
        for (int16_t i = currentSize - 1; i > index && i >= 0; i--) {
          targNode = targNode->prevNode;
        }
      } else {
        targNode = head;
        for (int16_t i = 0; i < index && i < LINKEDLIST_MAX_SIZE; i++) {
          targNode = targNode->nextNode;
        }
      }
      recentNode = targNode;
      recentIndex = index;
      return targNode;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Circular Buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

// Cycles after which loops will timeout...
const int16_t BUFFER_TIMEOUT = 2048; 

// @brief: Circular buffer of fixed size. 
// @NOTE: Keep in mind:
//   Strengths -> Storing a large amount of individual values/elements.
//             -> Able to use up very little ram -> if you pass it a pre-allocated array 
//                store elements/values into.
//  Weaknesses -> Inserting elements into the middle of the buffer (cannot be done).
//             -> Searching through buffer (cannot be done -> only beggining/end can be accessed).
//             -> Is not able to dynamically resize.
template<typename T> class Buffer {
  private:
    T *dataArray = nullptr; // Array that stores the buffer's data -> allocated on stack.
    int16_t size = 0; // Total capacity of the buffer's data array.
    int16_t writtenSize = 0; // Number of written to elements in the buffer.
    int16_t readIndex = 0; // Index of last read element.
    int16_t writeIndex = 1; // Index of next written element.
    bool adjustmentFlag = true; // Ensures all elements are read when read-head is lapped by write head.

  public:
    // #Constructor
    // @brief: Sets up the Buffer.
    // @param: *dataArray -> Ptr to the array that will store buffer's values.
    //               size -> The size of the passed array (& therefore buffer).
    Buffer(T *dataArray, int16_t size) {
        this->size = size;
        this->dataArray = dataArray;
        clear();
    }

    // @brief: Adds the specified value to the buffer.
    // @NOTE: If the buffer is full, elements will be overwritten.
    void add(T value) {
      dataArray[writeIndex] = value;
      // Have we lapped the read head?
      if (writeIndex == readIndex) {
        adjustmentFlag = true;
        readIndex++;
        if (readIndex == size) {
          readIndex = 0;
        }
      } // No - so move along...
      writeIndex++;
      if (writeIndex == size) {
        writeIndex = 0;
      }
      // Reset iterator.
      currentIndex = -1;
      writtenSize++;
    }

    // @brief: Removes & returns the next element in the buffer (FIFO).
    // @NOTE: If there is no values in the buffer this method will return an
    //        object constructed with the default constructor.
    T remove() {
      if (readIndex + 1 == writeIndex) {
        return T();
      } else if (readIndex == size - 1 && writeIndex == 0) {
        return T();
      } // We have not reached end of buffer.
      T targValue;
      // Do we have to read the current index or the next one?
      if (readIndex == writeIndex && adjustmentFlag) {
        adjustmentFlag = false;
        targValue = dataArray[readIndex];
      } else {
        readIndex++;
        if (readIndex == size) {
          readIndex = 0;
        }
        targValue = dataArray[readIndex];
      }
      // Reset iterator and return...
      currentIndex = -1;
      writtenSize--;
      return targValue;
    }

    // @brief: Counts the number of instances of a specific value in the buffer.
    // @param: The value to find in the buffer.
    // @return: # of instances of value in buffer.
    int16_t count(T value) {
      int16_t valueCount = 0;
      int16_t result = 0;
      int16_t timeoutCount = 0;
      resetNext();
      while(result != -1 && timeoutCount < BUFFER_TIMEOUT) {
        result = findNext(value);
        if (result != -1) {
          valueCount++;
        }
        timeoutCount++;
      }
      return valueCount;
    }

    // @brief: Find the distance of "value" from the front of the buffer.
    // @return: An int denoting the number of indicies away the specified
    //          value is from the front of the buffer.
    int16_t distanceTo(T value) {
      resetNext();
      int16_t absIndex = findNext(value);
      if (absIndex == -1) {
        return -1;
      } else if (absIndex > readIndex) {
        return absIndex - readIndex;
      } else {
        return ((size - readIndex) + absIndex);
      }
    }

    // @brief: True if the passed value is contained in the buffer,
    //         false otherwise.
    bool contains(T value) { return (count(value) != 0); }

    // @brief: Returns the size of the buffer (remaining elements
    //         that can be removed).
    int16_t currentSize() { return writtenSize; }

    // @brief: Returns the max values the buffer can store - it's max 
    //         capacity.
    int16_t capacity() { return size; }

    // @brief: Returns true if the buffer has elements, false otherwise.
    int16_t isEmpty() { return (size == 0); }

    // @brief: Empties out the buffer removing all added values.
    // @NOTE: This does not delete the buffer! Elements can still
    //        be added after this method is called!
    void clear() {
      for (int i = 0; i < size; i++) {
        dataArray[i] = T();
      }
    }

    // #Destructor
    ~Buffer() {}

  private:
    // @brief: Returns the index where the next instance of the specified
    //         value resides in the buffer.
    int16_t currentIndex = -1;
    int16_t findNext(T value) {
      if (readIndex < writeIndex) {
        if (currentIndex == -1) { 
          currentIndex = readIndex;
        }
        for (int16_t i = currentIndex; i < writeIndex; i++) {
          if (dataArray[i] == value) {
            currentIndex = i + 1;
            return i;
          }
        }
        currentIndex = -1;
        return -1;
      } else if (readIndex > writeIndex) {
        if (currentIndex == -1 || (currentIndex < readIndex && currentIndex > writeIndex)) {
          Serial.println("section1");
          currentIndex = 0;
        }
        for (int16_t i = currentIndex; i < size; i++) {
          if (i < readIndex && i >= writeIndex) {
            Serial.println("section2");
            i = readIndex;
          }
          if (dataArray[i] == value) {
            Serial.println("section3");
            currentIndex = i + 1;
            return i;
          }
        }
        currentIndex = -1;
        return -1;
      } else {
        if (currentIndex == -1) {
          currentIndex = 0;
        }
        for (int i = currentIndex; i < size; i++) {
          if (dataArray[i] == value) {
            currentIndex = i + 1;
            return i;
          }
        }
        currentIndex = -1;
        return -1;
      }
    }

    // @brief: Resets findNext method/iterator such that it begins at the start of
    //         the buffer next time it is called.
    void resetNext() { currentIndex = -1; }
};