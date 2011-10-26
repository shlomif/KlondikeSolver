/* Copyright (c) 2011 Matt Birrell
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include <stdio.h>
#include <time.h>
#include <sys/timeb.h>

const char RANKS[] = {"A23456789TJQK"};
const char SUITS[] = {"CDSH"};

enum Piles {
	WASTE = 0,
	TABLEAU1,
	TABLEAU2,
	TABLEAU3,
	TABLEAU4,
	TABLEAU5,
	TABLEAU6,
	TABLEAU7,
	STOCK,
	FOUNDATION1,
	FOUNDATION2,
	FOUNDATION3,
	FOUNDATION4
};

//Key value pair used in HashMap
struct Pair {
	char* key;
	int value,hash;
	Pair* next;
	Pair() { key = NULL; value = -1; hash = -1; next = NULL; }
	~Pair() { }
	Pair(char* key, int value, int hash, Pair* next) { this->key = key; this->value = value; this->next = next; this->hash = hash; }
};

class HashMap {
private:
	inline static bool equals(char* key1,char* key2) {
		while((*key1)!=0 && (*key2)!=0) {
			if((*key1)!=(*key2)) { return false; }
			++key1; ++key2;
		}
		return (*key1)==0 && (*key2)==0;
	}
public:
	int count,capacity,shift;
	Pair* table;

	HashMap(int shft) {
		count = 0;
		shift = shft;
		capacity = (1<<shift)-1;
		table = new Pair[capacity+1];
	}
	~HashMap() {
		clear();
		delete []table;
	}

	inline int size() { return count; }
	inline void clear() {
		count = 0;
		Pair* temp = table;
		Pair* next,*tmp;
		for (int j = 0; j <= capacity; ++j,++temp) {
			if (temp->key != NULL) {
				tmp = temp->next;
				while(tmp!=NULL) {
					delete []tmp->key;
					next = tmp->next;
					delete tmp;
					tmp = next;
				}
				delete []temp->key;
				temp->key = NULL;
				temp->next = NULL;
			}
		}
	}
	Pair* addGet(char* key, int value)
	{
		int hash = 0x55555555;
		int sft = 0;
		char* temp = key;
		while((*temp)!=0) {
			hash ^= ((*temp)<<sft)^sft;
			if((++sft)>=shift) { sft = 0; }
			++temp;
		}
		int i = hash & capacity;
		Pair* e = &table[i];
		while(e!=NULL && e->key!=NULL) {
			if(e->hash == hash && equals(e->key,key)) { delete []key; return e; }
			e = e->next;
		}
		++count;
		e = &table[i];
		Pair* ne = NULL;
		if(e->key!=NULL) { ne = new Pair(e->key,e->value,e->hash,e->next); }
		e->key = key;
		e->value = value;
		e->hash = hash;
		e->next = ne;
		return NULL;
	}
	//not implemented yet. not sure how usefull
	void resize(int newCapacity) {
		Pair* newTable = new Pair[newCapacity];
		Pair* newTemp = newTable;
		Pair* temp = table;
		for (int j = 0; j <= capacity; ++j,++temp) {
			if (temp->key != NULL) {
				newTemp->key = temp->key;
				newTemp->value = temp->value;
				newTemp->hash = temp->hash;
				newTemp->next = temp->next;
			}
		}
		delete[] table;
		table = newTable;
		capacity = newCapacity-1;
	}
};

//very fast random number generator I created
//randomness tested very well at http://www.cacert.at/random/
class Random {
private:
	int value,mix,twist;
	
	inline void CalculateNext() {
		int y = value ^ twist - mix ^ value;
		y ^= twist ^ value ^ mix;
		mix ^= twist ^ value;
		value ^= twist - mix;
		twist ^= value ^ y;
		value ^= (twist<<7) ^ (mix>>16) ^ (y<<8);
	}
public:
	Random() {
		timeb sTime;
		ftime(&sTime);
		setSeed((int)(sTime.time*1000L + sTime.millitm));
	}
	Random(int seed) { setSeed(seed); }

	void setSeed(int seed) {
		mix = 51651237;
		twist = 895213268;
		value = seed;
		for (int i = 0; i < 50; ++i) {
			CalculateNext();
		}
		seed ^= (seed >> 15);
		value = 0x9417B3AF ^ seed;
		for (int i = 0; i < 950; ++i) {
			CalculateNext();
		}
	}

	int next() {
		CalculateNext();
		return(value & 0x7fffffff);
	}
};

struct Card {
	int rank, suit, clr, odd, value, up;

	Card() { value = -1; rank = -1; suit = -1; up = 1; clr = 0; odd = 0; }
	Card(int hash) { value = hash; rank = hash % 13; suit = hash / 13; up = 0; clr = suit & 1; odd = rank & 1; }
	inline int hash() const { return (value<<2) | (up<<1) | 1; }
	inline void set(int hash) { value = hash; rank = hash % 13; suit = hash / 13; up = 0; clr = suit & 1; odd = rank & 1; }
	void print() const { printf("%c%c%c",(up? '+' : '-'),(rank>=0? RANKS[rank] : 'X'),(suit>=0? SUITS[suit] : 'X')); }
};

class Pile {
public:
	Card** cards;
	int size, top;//top represents index of bottom most faceup card. ie) if all cards are faceup it would be 0
	Pile() {
		cards = new Card*[24]; size = 0; top = -1;
		for(int i=0;i<24;++i) {
			cards[i] = NULL;
		}
	}
	inline void add(Card* card) { cards[size++] = card; card->up = 0; }
	inline void flip() {
		Card* temp = cards[size-1];
		if((temp->up = !temp->up)) {
			top = size-1;
			return;
		}
		top = -1;
	}
	inline int highValue() { return size > 0 ? cards[0]->value : -1; }
	inline Card* highCard() { return top >= 0 ? cards[top] : NULL; }
	inline int topRank() { return size > 0 ? cards[size - 1]->rank : -1; }
	inline int faceUpCount() { return top >= 0 ? size - top : 0; }
	inline void remove(Pile* to) {
		if(to->top<0) { to->top = to->size; }
		to->cards[to->size++] = cards[--size];
		if(top==size) { top = -1; }
	}
	inline void remove(Pile* to,int count) {
		if(to->top<0) { to->top = to->size; }
		for(int i=size-count; i<size; ++i) {
			to->cards[to->size++] = cards[i];
		}
		size -= count;
		if(top>=size) { top = -1; }
	}
	//used for the talon
	inline bool removeTop(Pile* to,int count,bool thru) {
		if(size>count || (size==count && !thru)) {
			int i = size-count;
			do {
				--size;
				cards[size]->up = !cards[size]->up;
				to->cards[to->size++] = cards[size];
			} while(size>i);
			return false;
		}
		count = to->size + size - count;
		do {
			--to->size; --count;
			to->cards[to->size]->up = !to->cards[to->size]->up;
			cards[size++] = to->cards[to->size];
		} while(count>0);
		return true;
	}
	inline void clear() { size = 0; top = -1; }
	
	void print() const {
		for(int i=size-1;i>=0;--i) {
			cards[i]->print();
		}
	}
};

struct Move {
	char from,to,cards;
	int val; //right now is used for multiple purposes. might need to name better.
	Move* next,*prev;
	
	Move() { from = -1; to = -1; cards = -1; val = 0; next = NULL; prev = NULL; }
	Move(char f,char t,char c,int v) { from = f; to = t; cards = c; val = v; next = NULL; prev = NULL; }
	~Move() { next = NULL; prev = NULL; }
	void print() const { printf("[%i %i %i %i]",from,to,cards,val); }
};
//simple linked list
class MoveList {
public:
	Move* first,*last,*extra;
	int size;
	
	MoveList() {
		size = 0;
		first = NULL;
		last = NULL;
		extra = NULL;
	}
	~MoveList() {
		Move* next,*temp = first;
		while(temp!=NULL) {
			next = temp->next;
			delete temp;
			temp = next;
		}
		trim();
	}
	
	Move* get(int pos) {
		Move* ret = first;
		while((--pos)>=0) { ret = ret->next; }
		return ret;
	}
	inline void clear() {
		if(first!=NULL) {
			last->next = extra;
			if(extra!=NULL) { extra->prev = last; }
			extra = first;
		}
		size = 0;
		first = NULL;
		last = NULL;
	}
	inline void trim() {
		Move* next,*temp = extra;
		while(temp!=NULL) {
			next = temp->next;
			delete temp;
			temp = next;
		}
	}
	void addLast(char fromPile, char toPile, char cardsMoved, int value) {
		++size;
		Move* temp;
		if (extra != NULL) {
			extra->from = fromPile; extra->to = toPile; extra->cards = cardsMoved; extra->val = value;
			temp = extra;
			extra = extra->next;
		} else {
			temp = new Move(fromPile, toPile, cardsMoved, value);
		}
		if (last != NULL) {
			last->next = temp;
			temp->prev = last;
			temp->next = NULL;
			last = temp;
			return;
		}
		first = temp;
		temp->prev = NULL;
		temp->next = NULL;
		last = first;
		return;
	}
	void addFirst(char fromPile, char toPile, char cardsMoved, int value) {
		++size;
		Move* temp;
		if (extra != NULL) {
			extra->from = fromPile; extra->to = toPile; extra->cards = cardsMoved; extra->val = value;
			temp = extra;
			extra = extra->next;
		} else {
			temp = new Move(fromPile, toPile, cardsMoved, value);
		}
		if (first != NULL) {
			temp->next = first;
			temp->prev = NULL;
			first->prev = temp;
			first = temp;
			return;
		}
		temp->prev = NULL;
		temp->next = NULL;
		first = temp;
		last = first;
		return;
	}
	void print() const {
		Move* temp = first;
		while(temp!=NULL) {
			temp->print();
			temp = temp->next;
		}
	}
	//this function is used to integrate into my java gui so I can visualize solutions
	void printPacked() const {
		Move* tmp = first;
		int f = 0,t;
		int ss = 24;
		int ws = 0;
		int val;
		while(tmp!=NULL) {
			val = tmp->val;
			while((--val)>=0) {
				if(ss==0) { ++f; ss = ws; ws = 0; }
				--ss; ++ws;
			}
			if(tmp->from==WASTE) { --ws; }
			f += 1 + tmp->val;
			tmp = tmp->next;
		}
		printf("%c%c",f/24+0x30,f%24+0x30);
		tmp = first;
		ss = 24; ws = 0;
		while(tmp!=NULL) {
			val = tmp->val;
			while((--val)>=0) {
				if(ss==0) {
					printf("%c%c%c",0x31,0x30,ws+0x30);
					ss = ws;
					ws = 0;
				}
				printf("%c%c%c",0x30,0x31,0x31);
				--ss; ++ws;
			}
			f = tmp->from;
			if(f==WASTE) { --ws; }
			t = tmp->to;
			f = (f<=TABLEAU7 && f>=TABLEAU1)? f+1 : (f==STOCK? WASTE : (f==WASTE? TABLEAU1 : f));
			t = (t<=TABLEAU7 && t>=TABLEAU1)? t+1 : (t==STOCK? WASTE : (t==WASTE? TABLEAU1 : t));
			printf("%c%c%c",f+0x30,t+0x30,tmp->cards+0x30);
			tmp = tmp->next;
		}
	}
};
enum MoveMasks {
	MOVE_USED = 0x10000000,
	MOVE_REQ = 0x20000000,
	MOVE_LAST = 0x40000000,
	MOVE_VALUE = 0x00ffffff
};
class MoveArray {
private:
	Move* store,*first,*last;
	MoveArray* lists;
	int capacity,open;

	MoveArray() { size = 0; open = 0; top = 0; capacity = 0; first = NULL; last = NULL; store = NULL; lists = NULL; }

	//used in sorting
	inline void addExisting(Move* move)
	{
		++size;
		if (last != NULL) {
			last->next = move;
			move->next = NULL;
			last = move;
			return;
		}
		first = move;
		move->next = NULL;
		last = first;
		return;
	}
public:
	int size,top;

	MoveArray(int length) { size = 0; open = 0; top = 0; capacity = length; first = NULL; last = NULL; store = NULL; lists = new MoveArray[65536]; resize(capacity); }
	~MoveArray() { delete []store; }
	
	inline void clear() { size = 0; top = 0; open = 0; first = NULL; last = NULL; }
	inline Move* get(int pos) { return &store[pos]; }
	//radix sort implementation
	void sort(bool descending) {
		if (size < 2) { return; }
		
		int desc = descending ? 65535 : 0;
		for(int i=0;i<65536;++i) { lists[i].clear(); }
		Move* temp,*next;
		for (int i = 0; i < 32; i += 16) {
			temp = first;
			while (temp != NULL) {
				int bucket = ((temp->val >> i) & 65535) - desc;
				if (bucket < 0) { bucket = -bucket; }
				next = temp->next;
				lists[bucket].addExisting(temp);
				temp = next;
			}
			first = NULL;
			last = NULL;
			size = 0;
			for (int j = 0; j < 65536; ++j) {
				if (lists[j].size>0) {
					temp = lists[j].first;
					while (temp != NULL) {
						next = temp->next;
						addExisting(temp);
						temp = next;
					}
					lists[j].first = NULL;
					lists[j].last = NULL;
				}
			}
		}
	}
	
	//Remove any moves not needed. Reopen top level moves.
	void prune() {
		Move* prev,*temp;
		temp = first;
		while(temp!=NULL) {
			if((temp->val&MOVE_USED)==0) {
				temp->val &= MOVE_VALUE;
				temp->val |= MOVE_REQ;
				prev = temp->prev;
				while(prev!=NULL && (prev->val&MOVE_REQ)==0) {
					prev->val |= MOVE_REQ;
					prev = prev->prev;
				}
				++top;
			}
			temp = temp->next;
		}
		temp = first;
		prev = NULL;
		while(temp!=NULL) {
			if((temp->val&MOVE_REQ)==0) {
				if(temp==first) {
					if(first-store<open) { open = (int)(first-store); }
					first = temp->next;
					temp->next = NULL;
					temp->prev = NULL;
					temp = first;
					if(temp==NULL) { last = NULL; }
				} else {
					if(temp-store<open) { open = (int)(temp-store); }
					last = temp->next;
					temp->next = NULL;
					temp->prev = NULL;
					temp = last;
					prev->next = temp;
				}
				--size;
			} else {
				prev = temp;
				temp->val &= ~MOVE_REQ;
				temp = temp->next;
			}
		}
		last = prev;
		sort(false);
	}
	//resize array if we ran out of room
	void resize(int length) {
		capacity = length;
		Move* newStack = new Move[capacity];
		if(store!=NULL) {
			Move* temp = newStack;
			Move* itr = store;
			int amt = size;
			while(amt>0) {
				temp->from = itr->from;
				temp->to = itr->to;
				temp->cards = itr->cards;
				temp->val = itr->val;
				temp->next = itr->next!=NULL? newStack+(itr->next-store) : NULL;
				temp->prev = itr->prev!=NULL? newStack+(itr->prev-store) : NULL;
				++temp; ++itr; --amt;
			}
			if(size>0) {
				first = newStack+(first-store);
				last = newStack+(last-store);
			}
			delete []store;
		}
		store = newStack;
	}
	inline int moveFirstToLast() {
		if(last!=first) {
			last->next = first;
			first = first->next;
			last = last->next;
			last->next = NULL;
		}
		last->val |= MOVE_LAST;
		--top;
		return (int)(last-store);
	}
	inline void setUsed(int pos) { (store+pos)->val |= MOVE_USED; }
	//add move to list and sort first few moves ascending
	int add(char fromPile,char toPile,char cardsMoved,int val,int pos=-1)
	{
		if(size+1>capacity) { resize(capacity*1.5); }
		++top; ++size;
		Move* temp = store+(open++);
		//fill in gaps
		while((store+open)->next!=NULL || last==(store+open)) { ++open; }
		temp->from = fromPile; temp->to = toPile; temp->cards = cardsMoved; temp->val = val; temp->prev = pos<0? NULL: store+pos;
		Move* cur;
		if (first != NULL) {
			if (val >= last->val) {
				last->next = temp;
				temp->next = NULL;
				last = temp;
				return size-1;
			}
			if (val <= first->val) {
				temp->next = first;
				first = temp;
			} else {
				cur = first;
				int amt = 0;
				int sort = 80+(top>>5);
				sort = sort>256?256:sort;
				while (amt<sort && cur->next != NULL && val > cur->next->val) { ++amt; cur = cur->next; }
				temp->next = cur->next;
				cur->next = temp;
			}
			if (last->next != NULL) { last = last->next; }
			return size-1;
		}
		temp->next = NULL;
		first = temp;
		last = first;
		return size-1;
	}
};