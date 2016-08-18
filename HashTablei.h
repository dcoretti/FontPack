#pragma once

// by: Danny Coretti (dcoretti@gmail.com)
//  Super simple open addressed linear probing hash table for integer mapping.
//  Table size is a power of two to allow bitwise operations and nodes have a reserved empty value of intmax
//  Simplified from version outlined here: http://preshing.com/20130605/the-worlds-simplest-lock-free-hash-table/
class HashTablei {
	static  constexpr float loadFactor = 0.75f;
	static const int defaultCapacity = 16;

	struct HTNodei {
		int key{ emptyVal };
		int val{ emptyVal };
	};

public:
	static const int emptyVal = 0x7FFFFFFF; // intmax

	HashTablei(int initialSize = defaultCapacity) : capacity(initialSize >> 1), numElements(0), table(nullptr) {
		resize();
	}
	~HashTablei() {
		delete[] table;
	}

	void put(int key, int val) {
		put(key, val, table);
	}

	int get(int key) {
		return get(key, table);
	}

	int remove(int key) {
		return remove(key, table);
	}

private:
	void put(int key, int val, HTNodei* hTable) {
		int i = probe(key, hTable);
		if (hTable[i].key == emptyVal) {
			numElements++;
		}
		hTable[i].key = key;
		hTable[i].val = val;

		if (numElements > resizeThreshold) {
			resize();
		}
	}

	int get(int key, HTNodei* hTable) {
		int i = probe(key, hTable);
		if (table[i].key == key) {
			return table[i].val;
		} else {
			return emptyVal;
		}
	}


	int remove(int key, HTNodei* hTable) {
		int i = probe(key, hTable);
		HTNodei &entry = hTable[i];
		int oldVal = entry.val;
		if (entry.key != emptyVal) {
			entry.key = emptyVal;
			entry.val = emptyVal;
		}
		return oldVal;
	}

	void resize() {
		// set capacity before re-insert to keep probe() working
		capacity <<= 1; // powers of two to give the ability to use bitwise and instead of %
		resizeThreshold = (int)((float)capacity * loadFactor);

		HTNodei *newTable = new HTNodei[capacity];
		if (table != nullptr) {
			for (int i = 0; i < capacity >> 1; i++) {
				if (table[i].key != emptyVal) {
					put(table[i].key, table[i].val, newTable);
				}
			}
		}

		delete[] table;
		table = newTable;
	}

	inline int probe(int key, HTNodei *hTable) {
		for (int i = hash(key);; i++) {
			i &= capacity - 1;
			if (hTable[i].key == emptyVal || hTable[i].key == key) {
				return i;
			}
		}
	}

	static int hash(int key) {
		// murmur3 hash finalizer
		//https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
		key ^= key >> 16;
		key *= 0x85ebca6b;
		key ^= key >> 13;
		key *= 0xc2b2ae35;
		key ^= key >> 16;
		return key;
	}

	HTNodei *table;
	int capacity;
	int numElements;
	int resizeThreshold;    // calculated at resize using load factor
};