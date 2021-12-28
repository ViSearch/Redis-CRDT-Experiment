# Conflict-Free Replicated Data Types Based on Redis

Several Conflict-Free Replicated Data Types (CRDTs) implemented based on Redis(6.0.5).

Things we do for such implementation:

* Enable Redis to replicate in P2P mode.
* Implement the CRDT framework.
* Implement specific CRDTs according to their algorithms.

The CRDTs currently implemented:

* Priority Queue
  * Add-Win PQ
  * Remove-Win PQ
  * RWF-PQ
* Set
  * Add-Win Set
  * Remove-Win Set
  * RWF-Set
* List
  * Remove-Win List
  * RWF-List

## Build

Our modified Redis is in folder *redis-6.0.5*. Please build it in the default mode:

```bash
cd redis-6.0.5
make
sudo make install
```

## CRDT Operations

Different implementations of the same type of CRDT offer the same operations for users. We use **[type][op]** to name our CRDT operations. The operations name contains of an implementation type prefix and an operation type suffix. For example, the name of operation of RWF-PQ to add one element is **rwfzadd**, where **rwf** is the implementation type prefix and **zadd** is the operation type suffix.

### PQ operations

The **[type]** prefix of different PQs are:

* Add-Win PQ : **o**
* Remove-Win PQ : **r**
* RWF-PQ : **rwf**

The operations of RPQs are:

* **[type]zadd Q E V** : Add a new element *E* into the priority queue *Q* with initial value *V*.
* **[type]zincby Q E I** : Add the increment *I* to the value of element *E* in the priority queue *Q*.
* **[type]zrem Q E** : Remove element *E* from the priority queue *Q*.
* **[type]zscore Q E** : Read the value of element *E* from the priority queue *Q*.
* **[type]zmax Q** : Read the element with the largest value in the priority queue *Q*. Returns the element and its value.

### Set operations

The **[type]** prefix of different Sets are:

* Add-Win Set : **o**
* Remove-Win Set : **r**
* RWF-Set : **rwf**

The operations of Sets are:

* **[type]sadd S E** : Add a new element *E* into the Set *S*.
* **[type]srem S E** : Remove element *E* from the Set *S*.
* **[type]scontains S E** : Read the existence of element *E* from the Set *S*.
* **[type]ssize S** : Returns the number of the element in the Set *S*.

### List operations

The **[type]** prefix of different Lists are:

* Remove-Win List : **r**
* RWF-List : **rwf**

The operations of Lists are:

* **[type]linsert L prev id content font size color property** : Add a new element *id* after *prev* into the list *L*, with the content *content*, and the initial properties *font* *size* *color* and *property*(bitmap that encodes bold, italic and underline). If *prev* is "null", it means insert the element at the beginning of the list. If *prev* is "readd", it means to re-add the element that is previously added and then removed, and restore its position.
* **[type]lupdate L id type value** : Update the *type* property with *value* for element *id* in list *L*.
* **[type]lrem L id** : Remove element *id* from the list *L*.
