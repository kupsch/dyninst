/*
 *  Copyright 1993 Jeff Hollingsworth.  All rights reserved.
 *
 */

/*
 * list.h - list ADT
 *
 * $Log: List.h,v $
 * Revision 1.7  1994/01/19 20:46:17  hollings
 * guardef defn of true/false.
 *
 * Revision 1.6  1993/12/15  21:06:54  hollings
 * removed destructors.  Our current list semantics don't support auto
 * destruction of list comonents since list elements can be shared between
 * lists.
 *
 * Revision 1.5  1993/12/13  20:11:14  hollings
 * added destructor for List class.
 *
 * Revision 1.4  1993/10/19  15:31:52  hollings
 * assorted small fixes.
 *
 * Revision 1.3  1993/08/02  22:46:37  hollings
 * added remove which was missing.
 *
 * Revision 1.2  1993/07/01  17:02:36  hollings
 * ansi endif comments
 *
 * Revision 1.1  1993/05/07  20:21:15  hollings
 * Initial revision
 *
 * Revision 1.1  1993/03/19  22:51:05  hollings
 * Initial revision
 *
 *
 */
#ifndef LIST_H
#define LIST_H
#include <stdio.h>
#include <stdlib.h>

typedef char Boolean;

#ifndef FALSE
#define FALSE 0
#define TRUE  1
#endif

inline static int ListHash(void *ptr, int size)
{
    return(((int)(ptr) % (int)(size)));
}

template <class Type> class List;

template <class Type> class ListItem {
    friend class List<Type>;
    friend class StringList<Type>;
    private:
	Type		data;
	void		*key;
	ListItem<Type>	*next;
};

template <class Type> class List {
    public:
	List() { head = NULL; }
	void add(Type data, void *key);
	void add(Type data) { add(data, (void *) data); }
	Boolean addUnique(Type data) { return(addUnique(data, (void *) data)); }
	Boolean addUnique(Type data, void *key) {
	    Type temp;

	    temp = find(key);
	    if (!temp) {
		add(data, key);
		return(TRUE);
	    } else {
		return(FALSE);
	    }
	}
	Type find(void *key);
	Boolean remove(void *key);
	int count()	{
	    int c;
	    ListItem<Type> *curr;

	    for (curr=head,c=0; curr; curr=curr->next) c++;
	    return(c);
	}
	Type operator *() { 
	    if (head) 
		return(head->data); 
	    else
		return((Type) NULL);
	}
	void operator +=(List<Type> mergee) {
	    ListItem<Type> *curr;

	    for (curr=mergee.head; curr; curr=curr->next) {
		add(curr->data, curr->key);
	    }
	}
	Type operator ++() { 
	    Type ret = (Type) NULL;
	    if (head) {
		ret = head->data;
		head = head->next; 
	    }
	    return(ret); 
	}
    protected:
	ListItem<Type>	*head;
};

template <class Type> void List<Type>::add(Type data, void *key)
{
    ListItem<Type> *ni;

    ni = new(ListItem<Type>);
    ni->data = data;
    ni->key = key;

    ni->next = head;
    head = ni;
}

template <class Type> Boolean List<Type>::remove(void *key)
{
    ListItem<Type> *lag;
    ListItem<Type> *curr;

    for (curr=head, lag = NULL; curr; curr=curr->next) {
	if (curr->key == key) {
	    break;
	}
	lag = curr;
    }

    if (curr) {
	if (lag) {
	    lag->next = curr->next;
	} else {
	    head = curr->next;
	}
	delete(curr);
	return(TRUE);
    } else {
	return(FALSE);
    }
}

template <class Type> Type List<Type>::find(void *data)
{
    ListItem<Type> *curr;

    for (curr=head; curr; curr=curr->next) {
	if (curr->key == data) {
	    return(curr->data);
	}
    }
    return((Type) 0);
}

template <class Type> class HTable {

    public:
	HTable(Type data) { HTable(); add(data, (void *) data); }
	HTable(); 
	void add(Type data, void *key);
	Boolean addUnique(Type data, void *key) {
	    Type temp;

	    temp = find(key);
	    if (temp ) {
		return(0);
	    } else {
		add(data, key);
		return(1);
	    }
	}
	Type find(void *key);
	Boolean remove(void *key);
        Type operator *() {
            Type curr;

            curr = *currList;
            if (curr) return(curr);
            for (currHid++; currHid < tableSize; currHid++) {
                if (table[currHid]) {
                    currList = *table[currHid];
                    curr = *currList;
                    if (curr) return(curr);
                }
            }
            return(NULL);
        }
	Type operator ++() {
	    Type curr;

	    curr = *currList;
	    currList++;
	    return(curr);
	}
	int count()	{
	    int i, total;

	    total = 0;
	    for (i=0; i < tableSize; i++) {
		if (table[i]) {
		    total += table[i]->count();
		}
	    }
	    return(total);
	}
    private:
	List<Type> **table;
	List<Type> currList;
	int currHid;
	int tableSize;
};

template <class Type> HTable<Type>::HTable()
{ 
    table = NULL;
    currHid = 0;
    tableSize = 0;
}

template <class Type> Type HTable<Type>::find(void *key)
{
    int hid;

    if (!tableSize) return(NULL);
    hid = ListHash(key, tableSize);
    if (!table[hid]) {
	return(NULL);
    } else {
	return(table[hid]->find(key));
    }
}

template <class Type> void HTable<Type>::add(Type data, void *key)
{
    int hid;

    if (!tableSize) {
	tableSize = 97;
	table = (List<Type>**) calloc(tableSize, sizeof(List<Type>*));
    }
    hid = ListHash(key, tableSize);
    if (!table[hid]) {
	table[hid] = new(List<Type>);
    }
    table[hid]->add(data, key);
}


template <class Type> Boolean HTable<Type>::remove(void *key)
{
    int hid;

    hid = ListHash(key, tableSize);
    if (table[hid]) {
	return(table[hid]->remove(key));
    }
    return(FALSE);
}

template <class Type> class StringList: public List<Type> {
    public:
	Type find(void *key);
};

template <class Type> Type StringList<Type>::find(void *data) 
{
    ListItem<Type> *curr;

    for (curr=head; curr; curr=curr->next) {
	if (!strcmp((char *) curr->key, (char *) data)) {
	    return(curr->data);
	}
    }
    return((Type) 0);
}

#endif /* LIST_H */
