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
#include "util.h"

class Solitaire {
private:
	int order[7]; //used for pile sorting
public:
	Random random;
	Card* cards;
	Pile* piles;
	MoveList moves; //list of moves currently available in the current state
	int redMin,blackMin; //minimum rank in foundation for red/black
	int rounds; //times through deck/talon
	int foundationCount; //cards in foundation
	
	Solitaire() {
		random = Random();
		cards = new Card[52];
		piles = new Pile[13];
		moves = MoveList();
		for(int i=0;i<52;++i) {
			cards[i].set(i);
		}
		reset();
	}
	~Solitaire() { delete []cards; delete []piles; }

	//put the game back to its initial state
	inline void reset() {
		redMin = -1; blackMin = -1; rounds = 0; foundationCount = 0;
		for(int i=0;i<13;++i) { piles[i].clear(); }
		for(int j=TABLEAU1,i=0;j<=TABLEAU7;++j) {
			for(int k=j;k<=TABLEAU7;++k,++i) { piles[k].add(&cards[i]); }
		}
		for(int i=51;i>=28;--i) { piles[STOCK].add(&cards[i]); }
		for(int i=TABLEAU1;i<=TABLEAU7;++i) { piles[i].flip(); }
	}
	//generate an array of characters that represent the state of the game
	char* key()
	{
		order[0] = TABLEAU1; order[1] = TABLEAU2;
		order[2] = TABLEAU3; order[3] = TABLEAU4;
		order[4] = TABLEAU5; order[5] = TABLEAU6;
		order[6] = TABLEAU7;
		int cur = 1;
		int fuc = piles[cur].size;
		//sort the piles
		while (cur < 7) {
			int curT = cur;
			do {
				if (piles[order[curT - 1]].highValue() <= piles[order[curT]].highValue()) { break; }
				int temp = order[curT - 1];
				order[curT - 1] = order[curT];
				order[curT--] = temp;
			} while (curT > 0);
			++cur;
			fuc += piles[cur].faceUpCount();
		}
		char* comp = new char[12 + fuc];
		int z = 0;
		comp[z++] = piles[0].size>0?piles[0].cards[piles[0].size-1]->value+1:1;
		comp[z++] = piles[8].size>0?piles[8].cards[piles[8].size-1]->value+1:1;
		comp[z++] = ((piles[FOUNDATION1].size + 1) << 4) | (piles[FOUNDATION2].size + 1);
		comp[z++] = ((piles[FOUNDATION3].size + 1) << 4) | (piles[FOUNDATION4].size + 1);
		Pile* pile;
		for(int i=0;i<7;++i) {
			pile = &piles[order[i]];
			if (pile->top >= 0) {
				for (int i = pile->top; i < pile->size; ++i) {
					comp[z++] = pile->cards[i]->value+1;
				}
			}
			comp[z++] = 120-pile->top;
		}
		comp[z] = 0;
		return comp;
	}
	//make a series of moves
	void makeMove(Move* move) {
		do {
			if(move->from!=move->to) {
				if(move->val>0) {
					//we have to draw cards off the stock
					if(piles[STOCK].removeTop(&piles[WASTE],move->val,false)) { ++rounds; }
				}
				if(move->cards==1) {
					piles[move->from].remove(&piles[move->to]);
					if(move->to>=FOUNDATION1) {
						++foundationCount;
						setFoundationMin();
					} else if(move->from>=FOUNDATION1) {
						--foundationCount;
						setFoundationMin();
					}
				} else {
					piles[move->from].remove(&piles[move->to],move->cards);
				}
			} else {
				piles[move->from].flip();
			}
			move = move->next;
		} while(move!=NULL);
	}
	//make a single move
	bool makeMove(int from,int to,int cards,int val) {
		bool thru = false;
		if(from!=to) {
			if(val>0) {
				if(piles[STOCK].removeTop(&piles[WASTE],val,false)) { ++rounds; thru = true; }
			}
			if(cards==1) {
				piles[from].remove(&piles[to]);
				if(to>=FOUNDATION1) {
					++foundationCount;
					setFoundationMin();
				} else if(from>=FOUNDATION1) {
					--foundationCount;
					setFoundationMin();
				}
			} else {
				piles[from].remove(&piles[to],cards);
			}
		} else {
			piles[from].flip();
		}
		return thru;
	}
	//undo a single move. thru is set to true if we made a move that increased the number of rounds.
	void undoMove(int from,int to,int cards,int val,bool thru) {
		if(from!=to) {
			if(cards==1) {
				piles[to].remove(&piles[from]);
				if(to>=FOUNDATION1) {
					--foundationCount;
					setFoundationMin();
				} else if(from>=FOUNDATION1) {
					++foundationCount;
					setFoundationMin();
				}
			} else {
				piles[to].remove(&piles[from],cards);
			}
			if(val>0) {
				if(piles[WASTE].removeTop(&piles[STOCK],val,thru)) { --rounds; }
			}
		} else {
			piles[to].flip();
		}
	}
	//determine available moves.
	void updateMoves() {
		moves.clear();

		//Check flip of tableau pile
		//Check tableau to foundation
		//Check tableau to tableau
		Pile* pile1 = &piles[TABLEAU1];
		Card* card1;
		for (int i = TABLEAU1; i <= TABLEAU7; ++i,++pile1)
		{
			int pile1Size = pile1->size;
			if (pile1Size == 0) { continue; }
			
			card1 = pile1->cards[pile1Size - 1];
			if (!card1->up) { moves.addLast(i, i, 0, 0); return; }
		}
		int wasteSize = piles[WASTE].size;
		int stockSize = piles[STOCK].size;
		
		//below section is logic used to tell if a whole tableau pile should be moved or not
		int amt = -5;
		bool stockKing = false;
		pile1 = &piles[STOCK];
		for(int i=0;i<stockSize;++i) { if(pile1->cards[i]->rank==12) { stockKing = true; break; } }
		pile1 = &piles[WASTE];
		for(int i=0;!stockKing && i<wasteSize;++i) { if(pile1->cards[i]->rank==12) { stockKing = true; break; } }

		pile1 = &piles[TABLEAU1];
		Pile* pile2;
		for (int i = TABLEAU1; i <= TABLEAU7; ++i,++pile1) {
			int pile1Size = pile1->size;
			if (pile1Size == 0) { continue; }

			card1 = pile1->cards[pile1Size - 1];
			int cardFoundation = 9+card1->suit;
			if (card1->rank - piles[cardFoundation].topRank()== 1) {
				//logic used to tell if we can safely move a card to its foundation
				//this logic should only be used here and not on the talon unless we are only drawing 1 card
				int min = (card1->clr == 0 ? redMin : blackMin)+2;
				if (card1->rank <= min) { moves.clear(); moves.addLast(i, cardFoundation, 1, 0); return; }
				moves.addLast(i, cardFoundation, 1, 0);
			}
			Card* card2 = pile1->cards[pile1->top];
			int pile1Length = (card2->rank - card1->rank + 1);
			bool kingMoved = false;
			pile2 = &piles[TABLEAU1];
			for (int j = TABLEAU1; j <= TABLEAU7; ++j,++pile2) {
				if (i == j) { continue; }

				int pile2Size = pile2->size;
				if (pile2Size == 0) {
					if (card2->rank != 12 || pile1Size == pile1Length || kingMoved) { continue; }
					moves.addLast(i, j, pile1Length, 0);
					//only create one move for a blank spot
					kingMoved = true;
					continue;
				}

				Card* card3 = pile2->cards[pile2Size - 1];
				//logic used to determine if a pile of cards can be moved ontop of another pile of cards
				if (card1->rank >= card3->rank || card2->rank + 1 < card3->rank || ((card3->clr ^ card1->clr) ^ (card3->odd ^ card1->odd)) != 0) { continue; }
				int pile1Moved = (card3->rank - card1->rank);

				if (pile1Moved == pile1Length) {//we are moving all face up cards
					if (pile1Size == pile1Length) {//we are moving all cards on this pile
						if (amt == -5) {
							//look for kings and empty spaces
							amt = stockKing ? -1 : 1;
							Pile* pile3 = &piles[TABLEAU1];
							for (int z = TABLEAU1; z <= TABLEAU7; ++z,++pile3) {
								if (pile3->size == 0) {
									amt = 1; break;
								} else if (pile3->top == 0) {
									if (pile3->cards[0]->rank != 12) {
										if (amt < 0) { amt = 0; break; }
										amt = 2;
									}
								} else if (pile3->top > 0) {
									if (amt > 1) { amt = 0; break; }
									amt = -1;
								}
							}
						}
						if (amt != 0) { continue; }
						if (stockKing) { moves.addLast(i, j, pile1Moved, 0); continue; }
						
						//look for kings in the tableau
						for (int z = TABLEAU1; z <= TABLEAU7; z++) {
							if (z == i) { continue; }
							if (piles[z].topRank() == 12 && piles[z].top > 0) { moves.addLast(i, j, pile1Moved, 0); break; }
						}
						continue;
					}
					moves.addLast(i, j, pile1Moved, 0); continue;
				}

				//look to see if we are covering a card that can be moved to the foundation
				card3 = pile1->cards[pile1Size-pile1Moved-1];
				if (card3->rank - piles[card3->suit+9].topRank() == 1) { moves.addLast(i, j, pile1Moved, 0); continue; }
			}
		}

		//Check waste to foundation
		//Check waste to tableau
		if (wasteSize > 0) {
			card1 = piles[WASTE].cards[wasteSize - 1];
			int wasteFoundation = 9+card1->suit;
			if (card1->rank - piles[wasteFoundation].topRank() == 1) {
				//should always add here if draw count is greater than 1
				int min = (card1->clr == 0 ? redMin : blackMin)+2;
				if (card1->rank <= min) { moves.clear(); moves.addLast(WASTE, wasteFoundation, 1, 0); return; }
				moves.addLast(WASTE, wasteFoundation, 1, 0);
			}
			pile1 = &piles[TABLEAU1];
			for (int i = TABLEAU1; i <= TABLEAU7; ++i,++pile1) {
				int size = pile1->size;
				if (size != 0) {
					Card* card = pile1->cards[size - 1];
					if (!card->up || card->rank - card1->rank != 1 || card->clr == card1->clr) { continue; }
					moves.addLast(WASTE, i, 1, 0);
					continue;
				}
				if (card1->rank != 12) { continue; }
				moves.addLast(WASTE, i, 1, 0);
				break;
			}
		}
		//check foundation to tableau
		//very rarely needed to solve optimally
		pile1 = &piles[FOUNDATION1];
		for (int i = FOUNDATION1; i <= FOUNDATION4; ++i,++pile1) {
			int size = pile1->size;
			if (size == 0) { continue; }

			card1 = pile1->cards[size - 1];
			int min = (card1->clr == 0 ? redMin : blackMin)+2;
			if (card1->rank <= min) { continue; }
			pile2 = &piles[TABLEAU1];
			for (int j = TABLEAU1; j <= TABLEAU7; ++j,++pile2) {
				int pile2Size = pile2->size;
				if(pile2Size==0) {
					if(card1->rank==12) { moves.addLast(i, j, 1, 0); break; }
					continue;
				}
				Card* card2 = pile2->cards[pile2Size - 1];

				if (!card2->up || card2->rank - card1->rank != 1 || card1->clr == card2->clr) { continue; }
				moves.addLast(i, j, 1, 0);
			}
		}
		//check cards waiting to be turned over from stock
		pile1 = &piles[STOCK];
		for (int j = stockSize - 1; j >= 0; --j) {
			card1 = pile1->cards[j];
			int stockFoundation = 9+card1->suit;
			if (card1->rank - piles[stockFoundation].topRank() == 1) {
				int min = (card1->clr ==0 ? redMin : blackMin)+2;
				if (card1->rank <= min) { moves.addLast(WASTE, stockFoundation, 1, stockSize - j); return; }
				moves.addLast(WASTE, stockFoundation, 1, stockSize - j);
			}
			pile2 = &piles[TABLEAU1];
			for (int i = TABLEAU1; i <= TABLEAU7; ++i,++pile2) {
				int size = pile2->size;
				if (size != 0) {
					Card* card = pile2->cards[size - 1];
					if (!card->up || card->rank - card1->rank != 1 || card->clr == card1->clr) { continue; }
					moves.addLast(WASTE, i, 1, stockSize - j);
					continue;
				}
				if (card1->rank != 12) { continue; }
				moves.addLast(WASTE, i, 1, stockSize - j);
				break;
			}
		}
		//check cards already turned over in the waste
		//meaning we have to "redeal" the deck
		pile1 = &piles[WASTE];
		--wasteSize;
		for (int j = 0; j < wasteSize; ++j) {
			card1 = pile1->cards[j];
			int stockFoundation = 9+card1->suit;
			if (card1->rank - piles[stockFoundation].topRank() == 1) {
				int min = (card1->clr == 0 ? redMin : blackMin)+2;
				if (card1->rank <= min) { moves.addLast(WASTE, stockFoundation, 1, stockSize + j + 1); return; }
				moves.addLast(WASTE, stockFoundation, 1, stockSize + j + 1);
			}
			pile2 = &piles[TABLEAU1];
			for (int i = TABLEAU1; i <= TABLEAU7; ++i,++pile2) {
				int size = pile2->size;
				if (size != 0) {
					Card* card = pile2->cards[size - 1];
					if (!card->up || card->rank - card1->rank != 1 || card->clr == card1->clr) { continue; }
					moves.addLast(WASTE, i, 1, stockSize + j + 1);
					continue;
				}
				if (card1->rank != 12) { continue; }
				moves.addLast(WASTE, i, 1, stockSize + j + 1);
				break;
			}
		}
	}
	//set minimum ranks in foundation
	inline void setFoundationMin() {
		int one = piles[FOUNDATION2].topRank();
		int two = piles[FOUNDATION4].topRank();
		redMin = one<=two? one : two;
		one = piles[FOUNDATION1].topRank();
		two = piles[FOUNDATION3].topRank();
		blackMin = one<=two? one : two;
	}
	//heuristic function used to determine lower bound of moves needed
	int minWinAt() {
		int win = (piles[STOCK].size<<1) + piles[WASTE].size; //needs to modified slightly for draw counts greater than 1
		Card* ctmp2;
		Pile* p = &piles[WASTE];
		Card* ctmp1;
		for(int i=p->size - 1; i >= 0; --i) {
			ctmp1 = p->cards[i];
			for(int j=i-1;j>=0;--j) {
				ctmp2 = p->cards[j];
				if(ctmp1->suit==ctmp2->suit && ctmp1->rank > ctmp2->rank) {
					++win; break;
				}
			}
		}
		p = &piles[TABLEAU1];
		for(int i=TABLEAU1;i<=TABLEAU7;++i,++p) {
			int temp = p->size;
			win += temp;
			int top = p->top;
			if(top<0) { top = temp; }
			win += top;
			while((--temp)>=0) {
				ctmp1 = p->cards[temp];
				for(int j=(top<temp?top-1 : temp-1);j>=0;--j) {
					ctmp2 = p->cards[j];
					if(ctmp1->suit==ctmp2->suit && ctmp1->rank > ctmp2->rank) {
						++win;
						if(top<temp) { temp = top; }
						break;
					}
				}
			}
		}
		return win;
	}
	int shuffle(int seed = -1) {
		if(seed!=-1) {
			random.setSeed(seed);
		} else {
			seed = random.next();
			random.setSeed(seed);
		}
		int temp;
		for(int x=0;x<250;++x) {
			int k = random.next() % 52;
			int j = random.next() % 52;
			temp = cards[k].value;
			cards[k].set(cards[j].value);
			cards[j].set(temp);
		}
		reset();
		return seed;
	}
	bool load(char* cardSet) {
		for(int i=0;i<52;i++) {
			int suit = (cardSet[i*3+2]^0x30)-1;
			if(suit<0 || suit>3) { return false; }
			if(suit>=2) { suit = (suit==2)? 3 : 2; }
			int rank = (cardSet[i*3]^0x30)*10 + (cardSet[i*3+1]^0x30) - 1;
			if(rank<0 || rank>12) { return false; }
			cards[i].set(suit*13+rank);
		}
		reset();
		return true;
	}
	void print() {
		for(int i=0;i<13;i++) {
			printf("%2i: ",i);
			piles[i].print();
			printf("\n");
		}
		moves.print();
		printf("\nMinWinAt: %i\n",minWinAt());
	}
	//IDA* implementation to solve specified deal
	int solve(int* max, bool show = false)
	{
		int bestF = 0,mm = *max;
		HashMap closed = HashMap(23);
		reset();
		closed.addGet(key(),minWinAt());
		MoveList mList = MoveList();
		MoveArray open = MoveArray(8000000);
		open.add(-1,-1,-1,minWinAt()<<12);
		int wa = 0,added = 0;
		while(open.top>0)
		{
			//grab first move and move it to the end so it can be cleaned up later.
			int parent = open.moveFirstToLast();
			Move* temp = open.get(parent);
			reset();
			mList.clear();
			wa = 0;
			//generate move list
			while (temp->cards >= 0) {
				mList.addFirst(temp->from, temp->to, temp->cards, temp->val & 31);
				wa += (temp->val&31)+1;
				temp = temp->prev;
			}
			//make all moves
			if(mList.first != NULL) { makeMove(mList.first); }
			//check and see if the game is won
			if (foundationCount > bestF || (foundationCount == 52 && wa <= mm)) {
				bestF = foundationCount;
				if (show) { printf("Depth: %i Open: %i-%i Closed: %i Foundation: %i\n",wa,open.size,open.top,closed.size(),bestF); }
				if (bestF == 52 && wa<=mm) {
					mList.printPacked(); printf("\n");
					closed.clear();
					*max = wa;
					return 52;
				}
			}
			//update list of available moves
			updateMoves();
			//check each of the available moves to see if it has been evaluated already or not
			temp = moves.first;
			added = 0;
			while (temp != NULL) {
				bool thru = makeMove(temp->from,temp->to,temp->cards,temp->val);
				int mvs = wa + temp->val + 1 + minWinAt();
				//only add moves with length less than current iteration depth
				if (mvs <= mm) {
					Pair* p = closed.addGet(this->key(),mvs);
					++added;
					//only add new moves or moves with fewer total moves (should just reupdate the existing move's parent, but havent got to it)
					if(p==NULL || p->value > mvs) {
						open.add(temp->from, temp->to, temp->cards,((52-foundationCount+rounds)<<5) | temp->val, parent);
						if(p!=NULL) { p->value = mvs; }	
					}
				}
				undoMove(temp->from,temp->to,temp->cards,temp->val,thru);
				temp = temp->next;
			}
			//if all branches from this parent have been added mark this move as no longer needed if we reopen the search
			if(added==moves.size) { open.setUsed(parent); }
			//reopen the search if we have not found a solution for the next higher depth
			if(open.top==0 && bestF<52) {
				if((++mm)>256) { return bestF; }
				*max = mm;
				int prevSize = open.size;
				open.prune();
				if (show) { printf("Reopening: %i OpenPrev: %i Open: %i-%i Closed: %i\n",mm,prevSize,open.size,open.top,closed.size()); }
			}
		}
		if (show) { printf("Failed. Open: %i-%i Closed: %i Foundation: %i\n",open.size,open.top,closed.size(),bestF); }
		closed.clear();
		return bestF;
	}
};

int main()
{
	Solitaire s = Solitaire();
	printf("Solitaire Solver 3.0 10/01/2011\n--------------------------------------------------------------------------------\n");
	bool loaded = true;
	int i = 0;
	FILE* f = fopen("deck.txt","r");
	char c1,c2 = ' ';
	char* cardset = new char[156];
	while(f!=NULL && fscanf(f,"%c",&c1)!=EOF) {
		if(c1=='/' && c2=='/') {
			while(fscanf(f,"%c",&c1)!=EOF && c1!='\n') {}
			c2 = ' ';
			continue;
		}
		c2 = c1;
		if(c1<0x30 || c1>0x39) { continue; }
		if(i<156) { cardset[i++] = c1; }
	}
	if(f!=NULL) { fclose(f); }
	
	if(i==0) {
		printf("No deck found to solve! Should be located in deck.txt\n");
		loaded = false;
	} else if(i<156 || !s.load(cardset)) {
		printf("Deck found in deck.txt is invalid. Please validate and try again.");
		loaded = false;
	}
	delete []cardset;
	if(loaded==false) { getchar(); return 0; }
	//s.load("092132014012091083053052082131102051021033122084062111094071081013103064041112093042113044104024124023074011054032133072031123134114043073063101121034022061");
	//s.shuffle();
	//s.print();
	timeb startTime;
	ftime(&startTime);
	i = s.minWinAt();
	printf("Trying %i\n",i);
	int x = s.solve(&i,true);
	printf("Found: %i %i\n",i,x);
	timeb endTime;
	ftime(&endTime);
	i = (endTime.time-startTime.time)*1000L+(endTime.millitm-startTime.millitm);
	printf("Done %i",i);
	getchar();
	return 0;
}