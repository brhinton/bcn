/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * lst.h
 *
 * Copyright (C) 2022,2023 Bryan Hinton
 * This file was merged from the following files and modified by Bryan Hinton.
 * include/asm-generic/rwonce.h
 * include/linux/compiler_types.h
 * include/linux/container_of.h
 * include/linux/list.h
 *
 */

#ifndef _LST_H
#define _LST_H
#include <stdbool.h>
#include <stddef.h>

#define typeof_member(T, m)	typeof(((T*)0)->m)

/**
 * ctr - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the ctr struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define ctr(ptr, type, member) ({				\
	void *__mptr = (void *)(ptr);					\
	static_assert(__same_type(*(ptr), ((type *)0)->member) ||	\
		      __same_type(*(ptr), void),			\
		      "pointer type mismatch in ctr()");	\
	((type *)(__mptr - offsetof(type, member))); })

/**
 * ctr_safe - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the ctr struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 * If IS_ERR_OR_NULL(ptr), ptr is returned unchanged.
 */
#define ctr_safe(ptr, type, member) ({				\
	void *__mptr = (void *)(ptr);					\
	static_assert(__same_type(*(ptr), ((type *)0)->member) ||	\
		      __same_type(*(ptr), void),			\
		      "pointer type mismatch in ctr_safe()");	\
	IS_ERR_OR_NULL(__mptr) ? ERR_CAST(__mptr) :			\
		((type *)(__mptr - offsetof(type, member))); })


#define __scalar_type_to_expr_cases(type)                               \
                unsigned type:  (unsigned type)0,                       \
                signed type:    (signed type)0

#define __unqual_scalar_typeof(x) typeof(                               \
                _Generic((x),                                           \
                         char:  (char)0,                                \
                         __scalar_type_to_expr_cases(char),             \
                         __scalar_type_to_expr_cases(short),            \
                         __scalar_type_to_expr_cases(int),              \
                         __scalar_type_to_expr_cases(long),             \
                         __scalar_type_to_expr_cases(long long),        \
                         default: (x)))

#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#define static_assert(expr, ...) __static_assert(expr, ##__VA_ARGS__, #expr)
#define __static_assert(expr, msg, ...) _Static_assert(expr, msg)

#ifndef READ_ONCE
#define READ_ONCE(x)  (*(const volatile __unqual_scalar_typeof(x) *)&(x))
#endif

#ifdef CONFIG_ILLEGAL_POINTER_VALUE
# define POISON_POINTER_DELTA _AC(CONFIG_ILLEGAL_POINTER_VALUE, UL)
#else
# define POISON_POINTER_DELTA 0
#endif

#define LST_POISON1  ((void *) 0x100 + POISON_POINTER_DELTA)
#define LST_POISON2  ((void *) 0x122 + POISON_POINTER_DELTA)
#define INIT	     ((void *) 0x022 + POISON_POINTER_DELTA)

struct lst_head {
	struct lst_head *next, *prev;
};

struct hlst_head {
	struct hlst_node *first;
};

struct hlst_node {
	struct hlst_node *next, **pprev;
};

#define LST_HEAD_INIT(name) { &(name), &(name) }

#define LST_HEAD(name) \
	struct lst_head name = LST_HEAD_INIT(name)

static inline void INIT_LST_HEAD(struct lst_head *lst)
{

	lst->next = lst;
	lst->prev = lst;
}

#ifdef CONFIG_DEBUG_LST
extern bool __lst_add_valid(struct lst_head *new,
                            struct lst_head *prev,
                            struct lst_head *next);
extern bool __lst_del_entry_valid(struct lst_head *entry);
#else
static inline bool __lst_add_valid(struct lst_head *new,
                                   struct lst_head *prev,
                                   struct lst_head *next)
{

	return true;
}
static inline bool __lst_del_entry_valid(struct lst_head *entry)
{

	return true;
}
#endif

static inline void __lst_add(struct lst_head *new,
                             struct lst_head *prev,
                             struct lst_head *next)
{

	if (!__lst_add_valid(new, prev, next))
		return;

	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void lst_add(struct lst_head *new, struct lst_head *head)
{

	__lst_add(new, head, head->next);
}

static inline void lst_add_tail(struct lst_head *new, struct lst_head *head)
{

	__lst_add(new, head->prev, head);
}

static inline void __lst_del(struct lst_head * prev, struct lst_head * next)
{

	next->prev = prev;
	prev->next = next;
}

static inline void __lst_del_clearprev(struct lst_head *entry)
{

	__lst_del(entry->prev, entry->next);
	entry->prev = NULL;
}

static inline void __lst_del_entry(struct lst_head *entry)
{

	if (!__lst_del_entry_valid(entry))
		return;

	__lst_del(entry->prev, entry->next);
}

static inline void lst_del(struct lst_head *entry)
{

	__lst_del_entry(entry);
	entry->next = LST_POISON1;
	entry->prev = LST_POISON2;
}

static inline void lst_replace(struct lst_head *old,
                               struct lst_head *new)
{

	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

static inline void lst_replace_init(struct lst_head *old,
                                    struct lst_head *new)
{

	lst_replace(old, new);
	INIT_LST_HEAD(old);
}

static inline void lst_swap(struct lst_head *entry1,
                            struct lst_head *entry2)
{

	struct lst_head *pos = entry2->prev;

	lst_del(entry2);
	lst_replace(entry1, entry2);
	if (pos == entry1)
		pos = entry2;
	lst_add(entry1, pos);
}

static inline void lst_del_init(struct lst_head *entry)
{

	__lst_del_entry(entry);
	INIT_LST_HEAD(entry);
}

static inline void lst_move(struct lst_head *lst, struct lst_head *head)
{

	__lst_del_entry(lst);
	lst_add(lst, head);
}

static inline void lst_move_tail(struct lst_head *lst,
                                 struct lst_head *head)
{

	__lst_del_entry(lst);
	lst_add_tail(lst, head);
}

static inline void lst_bulk_move_tail(struct lst_head *head,
                                      struct lst_head *first,
                                      struct lst_head *last)
{

	first->prev->next = last->next;
	last->next->prev = first->prev;

	head->prev->next = first;
	first->prev = head->prev;

	last->next = head;
	head->prev = last;
}

static inline int lst_is_first(const struct lst_head *lst,
                               const struct lst_head *head)
{

	return lst->prev == head;
}

static inline int lst_is_last(const struct lst_head *lst,
                              const struct lst_head *head)
{

	return lst->next == head;
}

static inline int lst_empty(const struct lst_head *head)
{

	return READ_ONCE(head->next) == head;
}

static inline void lst_rotate_left(struct lst_head *head)
{

	struct lst_head *first;

	if (!lst_empty(head)) {
		first = head->next;
		lst_move_tail(first, head);
	}
}

static inline void lst_rotate_to_front(struct lst_head *lst,
                                       struct lst_head *head)
{

	lst_move_tail(head, lst);
}

static inline int lst_is_singular(const struct lst_head *head)
{

	return !lst_empty(head) && (head->next == head->prev);
}

static inline void __lst_cut_position(struct lst_head *lst,
                                      struct lst_head *head, struct lst_head *entry)
{

	struct lst_head *new_first = entry->next;
	lst->next = head->next;
	lst->next->prev = lst;
	lst->prev = entry;
	entry->next = lst;
	head->next = new_first;
	new_first->prev = head;
}

static inline void lst_cut_position(struct lst_head *lst,
                                    struct lst_head *head, struct lst_head *entry)
{

	if (lst_empty(head))
		return;
	if (lst_is_singular(head) &&
	    (head->next != entry && head != entry))
		return;
	if (entry == head)
		INIT_LST_HEAD(lst);
	else
		__lst_cut_position(lst, head, entry);
}

static inline void lst_cut_before(struct lst_head *lst,
                                  struct lst_head *head,
                                  struct lst_head *entry)
{

	if (head->next == entry) {
		INIT_LST_HEAD(lst);
		return;
	}
	lst->next = head->next;
	lst->next->prev = lst;
	lst->prev = entry->prev;
	lst->prev->next = lst;
	head->next = entry;
	entry->prev = head;
}

static inline void __lst_splice(const struct lst_head *lst,
                                struct lst_head *prev,
                                struct lst_head *next)
{

	struct lst_head *first = lst->next;
	struct lst_head *last = lst->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

static inline void lst_splice(const struct lst_head *lst,
                              struct lst_head *head)
{

	if (!lst_empty(lst))
		__lst_splice(lst, head, head->next);
}

static inline void lst_splice_tail(struct lst_head *lst,
                                   struct lst_head *head)
{

	if (!lst_empty(lst))
		__lst_splice(lst, head->prev, head);
}

static inline void lst_splice_init(struct lst_head *lst,
                                   struct lst_head *head)
{

	if (!lst_empty(lst)) {
		__lst_splice(lst, head, head->next);
		INIT_LST_HEAD(lst);
	}
}

static inline void lst_splice_tail_init(struct lst_head *lst,
                                        struct lst_head *head)
{

	if (!lst_empty(lst)) {
		__lst_splice(lst, head->prev, head);
		INIT_LST_HEAD(lst);
	}
}

#define lst_entry(ptr, type, member) \
	ctr(ptr, type, member)

#define lst_first_entry(ptr, type, member) \
	lst_entry((ptr)->next, type, member)

#define lst_last_entry(ptr, type, member) \
	lst_entry((ptr)->prev, type, member)

#define lst_first_entry_or_null(ptr, type, member) ({ \
	struct lst_head *head__ = (ptr); \
	struct lst_head *pos__ = READ_ONCE(head__->next); \
	pos__ != head__ ? lst_entry(pos__, type, member) : NULL; \
})

#define lst_next_entry(pos, member) \
	lst_entry((pos)->member.next, typeof(*(pos)), member)

#define lst_prev_entry(pos, member) \
	lst_entry((pos)->member.prev, typeof(*(pos)), member)

#define lst_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define lst_for_each_continue(pos, head) \
	for (pos = pos->next; pos != (head); pos = pos->next)

#define lst_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

#define lst_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

#define lst_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	     pos != (head); \
	     pos = n, n = pos->prev)

#define lst_entry_is_head(pos, head, member)				\
	(&pos->member == (head))

#define lst_for_each_entry(pos, head, member)				\
	for (pos = lst_first_entry(head, typeof(*pos), member);	\
	     !lst_entry_is_head(pos, head, member);			\
	     pos = lst_next_entry(pos, member))

#define lst_for_each_entry_reverse(pos, head, member)			\
	for (pos = lst_last_entry(head, typeof(*pos), member);		\
	     !lst_entry_is_head(pos, head, member); 			\
	     pos = lst_prev_entry(pos, member))

#define lst_prepare_entry(pos, head, member) \
	((pos) ? : lst_entry(head, typeof(*pos), member))

#define lst_for_each_entry_continue(pos, head, member) 		\
	for (pos = lst_next_entry(pos, member);			\
	     !lst_entry_is_head(pos, head, member);			\
	     pos = lst_next_entry(pos, member))

#define lst_for_each_entry_continue_reverse(pos, head, member)		\
	for (pos = lst_prev_entry(pos, member);			\
	     !lst_entry_is_head(pos, head, member);			\
	     pos = lst_prev_entry(pos, member))

#define lst_for_each_entry_from(pos, head, member) 			\
	for (; !lst_entry_is_head(pos, head, member);			\
	     pos = lst_next_entry(pos, member))

#define lst_for_each_entry_from_reverse(pos, head, member)		\
	for (; !lst_entry_is_head(pos, head, member);			\
	     pos = lst_prev_entry(pos, member))

#define lst_for_each_entry_safe(pos, n, head, member)			\
	for (pos = lst_first_entry(head, typeof(*pos), member),	\
		n = lst_next_entry(pos, member);			\
	     !lst_entry_is_head(pos, head, member); 			\
	     pos = n, n = lst_next_entry(n, member))

#define lst_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = lst_next_entry(pos, member), 				\
		n = lst_next_entry(pos, member);				\
	     !lst_entry_is_head(pos, head, member);				\
	     pos = n, n = lst_next_entry(n, member))

#define lst_for_each_entry_safe_from(pos, n, head, member) 			\
	for (n = lst_next_entry(pos, member);					\
	     !lst_entry_is_head(pos, head, member);				\
	     pos = n, n = lst_next_entry(n, member))

#define lst_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = lst_last_entry(head, typeof(*pos), member),		\
		n = lst_prev_entry(pos, member);			\
	     !lst_entry_is_head(pos, head, member); 			\
	     pos = n, n = lst_prev_entry(n, member))

#define lst_safe_reset_next(pos, n, member)				\
	n = lst_next_entry(pos, member)

#define HLST_HEAD_INIT { .first = NULL }
#define HLST_HEAD(name) struct hlst_head name = {  .first = NULL }
#define INIT_HLST_HEAD(ptr) ((ptr)->first = NULL)
static inline void INIT_HLST_NODE(struct hlst_node *h)
{

	h->next = NULL;
	h->pprev = NULL;
}

static inline int hlst_unhashed(const struct hlst_node *h)
{

	return !h->pprev;
}

static inline int hlst_unhashed_lockless(const struct hlst_node *h)
{

	return !READ_ONCE(h->pprev);
}

static inline int hlst_empty(const struct hlst_head *h)
{

	return !READ_ONCE(h->first);
}

static inline void __hlst_del(struct hlst_node *n)
{

	struct hlst_node *next = n->next;
	struct hlst_node **pprev = n->pprev;

	*pprev = next;
	if (next)
		next->pprev = pprev;
}

static inline void hlst_del(struct hlst_node *n)
{

	__hlst_del(n);
	n->next = LST_POISON1;
	n->pprev = LST_POISON2;
}

static inline void hlst_del_init(struct hlst_node *n)
{

	if (!hlst_unhashed(n)) {
		__hlst_del(n);
		INIT_HLST_NODE(n);
	}
}

static inline void hlst_add_head(struct hlst_node *n, struct hlst_head *h)
{

	struct hlst_node *first = h->first;
	n->next = first;
	if (first)
		first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

static inline void hlst_add_before(struct hlst_node *n,
                                   struct hlst_node *next)
{

	n->pprev = next->pprev;
	n->next = next;
	next->pprev = &n->next;
	*(n->pprev) = n;
}

static inline void hlst_add_behind(struct hlst_node *n,
                                   struct hlst_node *prev)
{

	n->next = prev->next;
	prev->next = n;
	n->pprev = &prev->next;

	if (n->next)
		n->next->pprev = &n->next;
}

static inline void hlst_add_fake(struct hlst_node *n)
{

	n->pprev = &n->next;
}

static inline bool hlst_fake(struct hlst_node *h)
{

	return h->pprev == &h->next;
}

static inline bool
hlst_is_singular_node(struct hlst_node *n, struct hlst_head *h)
{

	return !n->next && n->pprev == &h->first;
}

static inline void hlst_move_lst(struct hlst_head *old,
                                 struct hlst_head *new)
{

	new->first = old->first;
	if (new->first)
		new->first->pprev = &new->first;
	old->first = NULL;
}

#define hlst_entry(ptr, type, member) ctr(ptr,type,member)

#define hlst_for_each(pos, head) \
	for (pos = (head)->first; pos ; pos = pos->next)

#define hlst_for_each_safe(pos, n, head) \
	for (pos = (head)->first; pos && ({ n = pos->next; 1; }); \
	     pos = n)

#define hlst_entry_safe(ptr, type, member) \
	({ typeof(ptr) ____ptr = (ptr); \
	   ____ptr ? hlst_entry(____ptr, type, member) : NULL; \
	})

#define hlst_for_each_entry(pos, head, member)				\
	for (pos = hlst_entry_safe((head)->first, typeof(*(pos)), member);\
	     pos;							\
	     pos = hlst_entry_safe((pos)->member.next, typeof(*(pos)), member))

#define hlst_for_each_entry_continue(pos, member)			\
	for (pos = hlst_entry_safe((pos)->member.next, typeof(*(pos)), member);\
	     pos;							\
	     pos = hlst_entry_safe((pos)->member.next, typeof(*(pos)), member))

#define hlst_for_each_entry_from(pos, member)				\
	for (; pos;							\
	     pos = hlst_entry_safe((pos)->member.next, typeof(*(pos)), member))

#define hlst_for_each_entry_safe(pos, n, head, member) 		\
	for (pos = hlst_entry_safe((head)->first, typeof(*pos), member);\
	     pos && ({ n = pos->member.next; 1; });			\
	     pos = hlst_entry_safe(n, typeof(*pos), member))

#endif
