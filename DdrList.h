// both directions ring list

#ifndef __BDRL_H__
#define __BDRL_H__

#ifdef __cplusplus
extern "C" {
#endif

// both directions node
typedef struct BDN
{
    struct BDN *ptNext;                          /**< point to next node. */
    struct BDN *ptPrev;                          /**< point to prev node. */
} t_BDN;
// both directions ring list
typedef t_BDN t_BDRL;

/**
 * @brief initialize a list
 *
 * @param ptList list to be initialized
 */
__inline void DdrListInit(t_BDRL *ptList)
{
    ptList->ptNext = ptList->ptPrev = ptList;
}

/**
 * @brief insert a node after a list
 *
 * @param ptList list to insert it
 * @param ptNode new node to be inserted
 */
__inline void DdrListInsertAfter(t_BDRL *ptList, t_BDN *ptNode)
{
    ptNode->ptNext = ptList->ptNext;
    ptNode->ptPrev = ptList;

    ptList->ptNext->ptPrev = ptNode;
    ptList->ptNext = ptNode;
}

/**
 * @brief insert a node before a list
 *
 * @param ptNode new node to be inserted
 * @param ptList list to insert it
 */
__inline void DdrListInsertBefore(t_BDRL *ptList, t_BDN *ptNode)
{
    ptNode->ptNext = ptList;
    ptNode->ptPrev = ptList->ptPrev;

    ptList->ptPrev->ptNext = ptNode;
    ptList->ptPrev = ptNode;
}

/**
 * @brief remove node from list.
 * @param ptNode the node to remove from the list.
 */
__inline void DdrListRemove(t_BDRL *ptNode)
{
    ptNode->ptNext->ptPrev = ptNode->ptPrev;
    ptNode->ptPrev->ptNext = ptNode->ptNext;

    ptNode->ptNext = ptNode->ptPrev = ptNode;
}

/**
 * @brief tests whether a list is empty
 * @param ptList the list to test.
 */
__inline int DdrListIsEmpty(const t_BDRL *ptList)
{
    return ptList->ptNext == ptList;
}

#ifndef offsetof
/**
 * @brief get the offset of member from the structure
 * @param s the type of structure
 * @param member the name of member in structure
 */
#define OffsetOf(s, m)   (int) &(((s *) 0)->m)
#endif

/**
 * @brief get the struct for this entry
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define ListEntry(list, type, member) \
    ((type *)((char *)(list) - (char *)OffsetOf(type, member)))

/**
 * @brief iterate through all nodes in the list
 * @param node
 * @param list
 */
#define EachNodeOfList(node, list)   \
    for((node) = (list)->ptNext; (node) != (list); (node) = (node)->ptNext)

	/**
	 * @brief iterate through all nodes's entry in the list
	 * @param entry 
	 * @param list
	 * @param member the name of list in structure
	 */
#define EachNodeEntryOfList(entry, list, member)                     \
	for((entry) = list_entry((list)->ptNext, typeof(*entry), member); &((entry)->member) != (list);	(entry) = list_entry((entry)->member.ptNext, typeof(*entry), member))

/*@}*/

#ifdef __cplusplus
}
#endif

#endif // __BDRL_H__

