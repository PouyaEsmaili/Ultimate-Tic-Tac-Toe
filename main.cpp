#include <iostream>
#include <cassert>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <chrono>
#include <ctime>

using namespace std;

// Types
using slowminiboard_t = int[9]; // Encoded as 0 1 or 2 for every position
using miniboard_t = int; // Encoded as a "Board position" in base 3
using board_t = miniboard_t[9]; // Encoded as 9 mini boards
using move_t = int; // Encoded as integer from 0 -> 81 indicated where to place a stone
using movelist_t = std::vector<move_t>;
struct mcnode_t;
typedef struct mcnode_t {
	mcnode_t *next;
	mcnode_t *child;
	mcnode_t *parent; // TODO use info from selection
	int visits, player;
	move_t mv;
	float mean, upper, invsqrtvisits;
} mcnode_t;

// Consts
const int BOARD_POSITIONS = 19683;
const int WHITE_WIN = 1;
const int BLACK_WIN = 2;
const int EGALITY = 0;
const int NOT_OVER = -1;
const int NULL_MOVE = -1;
const int MEMSIZE = 500'000'000;
const int OBJ_SIZE = MEMSIZE / sizeof(mcnode_t);
float FPU_C = 1.2f;
float C = 0.7f;

const int POW_THREE[9] = { 1, 3, 3 * 3, 3 * 3 * 3, 3 * 3 * 3 * 3, 3 * 3 * 3 * 3 * 3, 3 * 3 * 3 * 3 * 3 * 3, 3 * 3 * 3 * 3 * 3 * 3 * 3, 3 * 3 * 3 * 3 * 3 * 3 * 3 * 3 };
const int POPCNT[512] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8, 5, 6, 6, 7, 6, 7, 7, 8, 6, 7, 7, 8, 7, 8, 8, 9 };
int RD_POS[512 * 9];
int max_from_move[81]; // Get max board id for board_t from move_t
int min_from_move[81]; // Get which miniboard move was played on
move_t movegen_to_move[81];

movelist_t empty_from_miniboard[BOARD_POSITIONS]; // get empty spaces from miniboards
unsigned long long emptybits_from_miniboard[BOARD_POSITIONS]; // get empty spaces from miniboards
int nb_emptybits_from_miniboard[BOARD_POSITIONS]; // get empty spaces from miniboards

int state_from_miniboard[BOARD_POSITIONS]; // get win info on miniboard

void fast_to_slow(miniboard_t mini, slowminiboard_t value);
int get_winner(slowminiboard_t mini);

mcnode_t MEMORY[OBJ_SIZE];
int MEMORY_PTR = 0;

inline mcnode_t* allocate() {
	MEMORY_PTR = (MEMORY_PTR + 1) % OBJ_SIZE;
	return &MEMORY[MEMORY_PTR];
}

static unsigned long x = 123456789, y = 362436069, z = 521288629;

const int tab32[32] = {
		0,  9,  1, 10, 13, 21,  2, 29,
		11, 14, 16, 18, 22, 25,  3, 30,
		8, 12, 20, 28, 15, 17, 24,  7,
		19, 27, 23,  6, 26,  5,  4, 31 };

int firstlog1024[1024];

int log2_32(uint32_t value)
{
	if (value < 1024) {
		return firstlog1024[value];
	}
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	return tab32[(uint32_t)(value * 0x07C4ACDD) >> 27];
}

unsigned long fast_rand(void) {          //period 2^96-1
	unsigned long t;
	x ^= x << 16;
	x ^= x >> 5;
	x ^= x << 1;

	t = x;
	x = y;
	y = z;
	z = t ^ x ^ y;

	return z;
}

// Init
void init_board(board_t b) {
	for (int i = 0; i < 9; i++) {
		b[i] = 0;
	}
}

// Printing
void print_slowboard(slowminiboard_t mini) {
	for (int i = 0; i < 9; i++) {
		cerr << mini[i];
		if (i % 3 == 2) {
			cerr << endl;
		}
	}
}

void print_fastboard(miniboard_t mini) {
	slowminiboard_t val;
	fast_to_slow(mini, val);
	print_slowboard(val);
}

void print_moves(movelist_t moves) {
	for (unsigned int i = 0; i < moves.size(); i++) {
		cerr << moves[i] << " ";
	}
	cerr << endl;
}

void print_wholeboard(board_t mini) {
	slowminiboard_t vals[9];
	for (int i = 0; i < 9; i++) {
		fast_to_slow(mini[i], vals[i]);
	}
	for (int i = 0; i < 9; i++) {

		for (int j = 0; j < 9; j++) {
			int m = (i / 3) * 3 + j / 3;
			int val = vals[m][(i % 3) * 3 + j % 3];
			if (val > 0) {
				cerr << val;
			}
			else {
				cerr << " ";
			}
			if (j % 3 == 2) {
				cout << "|";
			}
		}
		if (i % 3 == 2) {
			cout << endl << "---+---+---";
		}
		cerr << endl;
	}
}


void print_wholeboard_filled(board_t mini) {
	slowminiboard_t vals[9];
	for (int i = 0; i < 9; i++) {
		fast_to_slow(mini[i], vals[i]);
	}
	for (int i = 0; i < 9; i++) {
		for (int j = 0; j < 9; j++) {
			int m = (i / 3) * 3 + j / 3;
			int stat = get_winner(vals[m]);
			int val = vals[m][(i % 3) * 3 + j % 3];
			if (stat > 0)
				val = stat;
			if (val > 0) {
				cerr << val;
			}
			else {
				cerr << " ";
			}
			if (j % 3 == 2) {
				cout << "|";
			}
		}
		if (i % 3 == 2) {
			cout << endl << "---+---+---";
		}
		cerr << endl;
	}
}

void print_mcnode(mcnode_t* node, int depth, int cutoff) {
	if (node->visits < cutoff)
		return;

	for (int i = 0; i < depth; i++) {
		cerr << "  ";
	}
	cerr << node->mv / 9 << "-" << node->mv % 9 << " " << node->mean << "/" << node->visits << " u: " << node->upper << endl;
	if (!node->child)
		return;
	mcnode_t* child = node->child;
	while (child) {
		//if (child->visits > 0) {
		print_mcnode(child, depth + 1, cutoff);
		//}
		child = child->next;
	}
}

// Type conversions
inline int get_pos(int delta, int mini) {
	return RD_POS[delta * 512 + mini];
}

miniboard_t slow_to_fast(slowminiboard_t mini) {
	miniboard_t value = 0;
	int power = 1;
	for (int i = 0; i < 9; i++) {
		value += mini[i] * power;
		power *= 3;
	}
	return value;
}

void fast_to_slow(miniboard_t mini, slowminiboard_t value) {
	for (int i = 0; i < 9; i++) {
		value[i] = mini % 3;
		mini /= 3;
	}
}

// Movegen
int get_winner(slowminiboard_t mini) {
	for (int i = 0; i < 3; i++) {
		if (mini[i] != 0 && mini[i] == mini[i + 3] && mini[i] == mini[i + 6]) {
			return mini[i];
		}
		if (mini[i * 3] != 0 && mini[i * 3] == mini[i * 3 + 1] && mini[i * 3] == mini[i * 3 + 2]) {
			return mini[i * 3];
		}
	}
	if (mini[4] != 0) {
		if (mini[0] == mini[4] && mini[0] == mini[8])
			return mini[4];
		if (mini[2] == mini[4] && mini[2] == mini[6])
			return mini[4];
	}
	for (int i = 0; i < 9; i++) {
		if (mini[i] == 0) {
			return NOT_OVER;
		}
	}
	return EGALITY;
}


void init_precalculations() {
	srand(0);
	for (int i = 0; i < 81; i++) {
		max_from_move[i] = i % 3 + 3 * ((i % 27) / 9);
		min_from_move[i] = (i % 9) / 3 + 3 * (i / 27);
		movegen_to_move[i] = max_from_move[i] + min_from_move[i] * 9;
	}

	for (int i = 1; i < 1024; i++) {
		firstlog1024[i] = (int)std::log2(i);
	}

	for (int i = 0; i < 9; i++) {
		for (int j = 0; j < 512; j++) {
			RD_POS[i * 512 + j] = -1;
		}
	}
	for (int j = 0; j < 512; j++) {
		int val = j;
		int counter = 0;
		for (int i = 0; i < 9; i++) {
			if (val & 1) {
				RD_POS[counter * 512 + j] = i;
				counter++;
			}
			val >>= 1;
		}
	}

	for (int i = 0; i < BOARD_POSITIONS; i++) {
		slowminiboard_t val;
		fast_to_slow(i, val);
		assert(slow_to_fast(val) == i);
		std::vector<int> moves = std::vector<int>();

		state_from_miniboard[i] = get_winner(val);
		int emptybits = 0;
		for (int j = 0; j < 9; j++) {
			if (val[j] == 0) {
				emptybits += 1 << j;
				moves.push_back(j % 3 + (j / 3) * 9);
			}
		}
		empty_from_miniboard[i] = moves;
		emptybits_from_miniboard[i] = emptybits;
		nb_emptybits_from_miniboard[i] = moves.size();
	}
}

// Returns nb of moves
inline int fast_moves(board_t board, move_t last_move, unsigned long long int& first_part, int& second_part) {
	int maxboard = max_from_move[last_move];
	miniboard_t maxboard_board = board[maxboard];
	int state = state_from_miniboard[maxboard_board];

	if (state == NOT_OVER) {
		unsigned long long movesbits = emptybits_from_miniboard[maxboard_board];
		if (maxboard < 7) {
			first_part |= movesbits << (9 * maxboard);
		}
		else {
			second_part |= movesbits << (9 * (maxboard - 7));
		}
		return nb_emptybits_from_miniboard[maxboard_board];
	}
	else {
		int nb = 0;
		for (int i = 0; i < 7; i++) {
			if (state_from_miniboard[board[i]] == NOT_OVER) {
				unsigned long long empties = emptybits_from_miniboard[board[i]];
				nb += nb_emptybits_from_miniboard[board[i]];
				first_part |= empties << (9 * i);
			}
		}
		for (int i = 7; i < 9; i++) {
			if (state_from_miniboard[board[i]] == NOT_OVER) {
				int empties = emptybits_from_miniboard[board[i]];
				nb += nb_emptybits_from_miniboard[board[i]];
				second_part |= empties << (9 * (i - 7));
			}
		}
		return nb;
	}
}

// Utils
const int calc[4] = { 0, 0, 1, 2 };
int get_status(board_t b) {
	int value = 0;
	int at_least_a_negative = 1;
	for (int i = 0; i < 9; i++) {
		int st = state_from_miniboard[b[i]];
		at_least_a_negative *= st + 1;
		value += calc[st + 1] * POW_THREE[i];
	}
	int rs = state_from_miniboard[value];

	if (rs >= 1)
		return rs;
	if ((rs == -1 && at_least_a_negative != 0) || rs == 0) {
		int w_a = 0;
		for (int i = 0; i < 9; i++) {
			int st = state_from_miniboard[b[i]];
			if (st > 0) {
				w_a += -2 * st + 3;
			}
		}
		if (w_a > 0) {
			return 1;
		}
		else if (w_a < 0) {
			return 2;
		}
		return 0;
	}
	return -1;
}

bool isEmpty(miniboard_t mini) {
	return mini == 0;
}

const int play_id_table[3] = { 1, 0, 2 };
void apply_move(board_t board, move_t mov, int player) {
	int maxb = max_from_move[mov];
	int play_id = play_id_table[player + 1];
	board[min_from_move[mov]] += POW_THREE[maxb] * play_id;
}

void undo_move(board_t board, move_t mov, int player) {
	int maxb = max_from_move[mov];
	int play_id = play_id_table[player + 1];
	board[min_from_move[mov]] -= POW_THREE[maxb] * play_id;
}

mcnode_t* pick_uct_node(mcnode_t* root) {
	mcnode_t* best = root->child;
	mcnode_t* iter = best;

	float upper = best->upper;

	while (iter->next) {
		iter = iter->next;
		float upper2 = iter->upper;
		if (upper2 > upper) {
			upper = upper2;
			best = iter;
		}
	}
	return best;
}

int nodes = 0;
mcnode_t* expand_nodes(mcnode_t* root, board_t b) {
	mcnode_t* child = allocate();
	mcnode_t* random_child = 0;

	unsigned long long int first_part = 0;
	int second_part = 0;
	int nb;
	if (root->mv == NULL_MOVE) {
		first_part = 0xFFFFFFFFFFFFFFFF;
		second_part = 0xFFFFFFF;
		nb = 81;
	}
	else {
		nb = fast_moves(b, root->mv, first_part, second_part);
	}
	int rd = rand() % nb;
	int cnt = -1;
	nodes += nb;

	root->child = child;

	if (first_part > 0) {
		for (int i = 0; i < 63; i++) {
			if ((first_part & (1ULL << i)) > 0) {
				cnt++;
				move_t mv = movegen_to_move[i];
				if (cnt == rd) {
					random_child = child;
				}
				child->child = nullptr;
				child->mv = mv;
				child->player = -root->player; // -1 <--> 1
				child->visits = 0;
				child->mean = 0;
				child->parent = root;
				child->upper = FPU_C + ((float)(rand()) / RAND_MAX) / 100.0f;
				if (cnt < nb - 1) {
					child->next = allocate();
					child = child->next;
				}
				else {
					child->next = 0;
				}
			}
		}
	}

	for (int i = 0; i < 18; i++) {
		if ((second_part & (1 << i)) > 0) {
			cnt++;
			move_t mv = movegen_to_move[63 + i];
			if (cnt == rd) {
				random_child = child;
			}
			child->child = nullptr;
			child->mv = mv;
			child->player = -root->player; // -1 <--> 1
			child->visits = 0;
			child->mean = 0;
			child->parent = root;
			child->upper = FPU_C + ((float)(rand()) / RAND_MAX) / 100.0f;
			if (cnt < nb - 1) {
				child->next = allocate();
				child = child->next;
			}
			else {
				child->next = 0;
			}
		}
	}

	return random_child;
}

move_t get_random_move(board_t board, move_t last_move, int player) {
	unsigned long long int first_part = 0;
	int second_part = 0;
	int rd;
	if (last_move == NULL_MOVE) {
		first_part = 0xFFFFFFFFFFFFFFFF;
		second_part = 0xFFFFFFF;
		rd = fast_rand() % 81;
	}
	else {
		rd = fast_rand() % fast_moves(board, last_move, first_part, second_part);
	}
	int cnt = 0;

	for (int i = 0; i < 7; i++) {
		int tmp = cnt;
		cnt += POPCNT[first_part & 0b111111111];
		if (cnt > rd) {
			return movegen_to_move[i * 9 + get_pos(rd - tmp, first_part & 0b111111111)];
		}
		first_part >>= 9;
	}

	for (int i = 0; i < 2; i++) {
		int tmp = cnt;
		cnt += POPCNT[second_part & 0b111111111];
		if (cnt > rd) {
			return movegen_to_move[63 + i * 9 + get_pos(rd - tmp, second_part & 0b111111111)];
		}
		second_part >>= 9;
	}

	return NULL_MOVE;
}

int simulate(mcnode_t* node, board_t board) {
	move_t last_move = node->mv;
	board_t cp;
	for (int j = 0; j < 9; j++)
		cp[j] = board[j];
	int player = -node->player;
	int status = get_status(board);
	while (status == NOT_OVER) {
		move_t rdmv = get_random_move(board, last_move, player);
		apply_move(board, rdmv, player);
		last_move = rdmv;
		player *= -1;
		status = get_status(board);
	}

	for (int j = 0; j < 9; j++)
		board[j] = cp[j];
	return status;
}

void do_playout(mcnode_t* node, board_t board) {
	// 1. Selection
	mcnode_t* root = node;
	while (true) {
		if (!node->child) { // is leaf
			break;
		}
		node = pick_uct_node(node);
		apply_move(board, node->mv, node->player);
	}

	// 2. Expand
	int status = get_status(board);
	mcnode_t* random_child;
	int result;
	if (status == NOT_OVER) {
		if(node->visits > 0)
		{
			random_child = expand_nodes(node, board);
			apply_move(board, random_child->mv, random_child->player);
		}
		else
			random_child = node;
		// 3. Simulation
		result = simulate(random_child, board); // TODO: Either win or lose ? Should be expected score maybe ? win draw lose..
		node = random_child;
	}
	else {
		result = status;// already have result
	}

	float val = 0;
	if (result == (3 + node->player) / 2) {
		val = 1.0f;
	}
	else if (result == 0) {
		val = 0.5f;
	}

	// 4. Backpropagation
	while (node != root) {
		undo_move(board, node->mv, node->player);
		node->visits += 1;
		node->mean += (val - node->mean) / node->visits;
		node->invsqrtvisits = 1 / std::sqrt(node->visits);

		float logpvis = std::sqrt(log2_32(node->parent->visits + 1));

		for (mcnode_t* i = node->parent->child; i; i = i->next) {
			if (i->visits > 0) {
				i->upper = i->mean + C * logpvis * i->invsqrtvisits;
			}
		}

		node = node->parent;
		val = 1 - val;
	}
	node->visits += 1;
	node->invsqrtvisits = 1 / std::sqrt(node->visits);
}

move_t pick_best_move(mcnode_t* root) {
	float max = -1;
	mcnode_t* child = root->child;
	mcnode_t* best = child;
	while (child) {
		if (child->mean > max) {
			max = child->mean;
			best = child;
		}
		child = child->next;
	}
	return best->mv;
}

int playouts = 0;
move_t get_best_move(board_t b, move_t last_move, int player) {
	auto tim = std::chrono::steady_clock::now();
    mcnode_t root;
	root.child = 0;
	root.mv = last_move;
	root.player = -player;
	root.visits = 0;
	root.mean = 0;
	for (playouts = 0;; playouts++) {
		if (playouts % 100 == 0) {
			if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tim).count() > 96) {
				break;
			}
		}
		do_playout(&root, b);
	}
	return pick_best_move(&root);
}

bool openingBook(board_t b, move_t last_move, int turn, move_t& to_play) {
	if (turn == 0) {
		to_play = 4 * 9 + 4;
		return true;
	}
	if (turn == 1 && last_move == 4 * 9 + 4) {
		to_play = 3 * 9 + 3;
		return true;
	}
	int maxb = max_from_move[last_move];
	if ((maxb == 0 || maxb == 2 || maxb == 6 || maxb == 8) && isEmpty(b[maxb])) {
		to_play = movegen_to_move[maxb * 9 + maxb];
		return true;
	}
	return false;
}

void play_CG() {
	board_t b;
	init_board(b);
	move_t last_move = NULL_MOVE;
	int player = 1;
	int turn = 0;
	bool still_in_book = true;
	while (1) {
		int opponentRow;
		int opponentCol;
		cin >> opponentRow >> opponentCol; //cin.ignore();
		int validActionCount;
		cin >> validActionCount; cin.ignore();

		for (int i = 0; i < validActionCount; i++) {
			int row;
			int col;
			cin >> row >> col; cin.ignore();
		}

		if (opponentRow != -1) {
			last_move = opponentRow * 9 + opponentCol;
			apply_move(b, last_move, player);
			player *= -1;
			turn += 1;
		}

		move_t move_taken;
		if (still_in_book) {
			still_in_book = openingBook(b, last_move, turn, move_taken);
		}
		if (!still_in_book) {
			nodes = 0;
			move_taken = get_best_move(b, last_move, player);
		}
		apply_move(b, move_taken, player);
		int col = move_taken % 9;
		int row = move_taken / 9;
		cout << row << " " << col << endl;

		player *= -1;
		turn += 1;
	}
}

void infinite_playouts_from_root() {
    auto tim = std::chrono::steady_clock::now();
    board_t b;
    init_board(b);
    move_t last_move = NULL_MOVE;
    mcnode_t root;
    root.child = 0;
    root.mv = NULL_MOVE;
    root.player = 1;
    root.visits = 0;
    root.mean = 0;
    for (playouts = 0;; playouts++) {
        if (playouts % 10000 == 0) {
            printf ("playouts: %d, time: %lld\n", playouts,  std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tim).count());
        }
        do_playout(&root, b);
    }
}

// Main
int main()
{
	init_precalculations();
	//infinite_playouts_from_root();
	play_CG();
}