#pragma once

#include "../string_view.hh"

#include <string>
#include <vector>

// linear probing w/ & operation for hashing.
namespace v4 {

class Dictionary
{
public:
	template<typename Collection>
	explicit Dictionary(const Collection& words)
	{
		std::size_t hashSize = 1;
		if ((words.size() & (words.size() - 1)) == 0)
			hashSize = words.size() * 2; // words.size()是2的n次方, 直接使用它作为hashSize的大小
		else
		{
			while (hashSize <= words.size())
			{
				// 为什么我们需要hashSize是2的n次方. 原因是与求模运算有关.
				// 如果hashSize是2的n次方. 那么求模的时候我们可以不用%运算
				// hash & (hashSize - 1), 这样的话效率会高很多
				hashSize <<= 1;
			}
		}

		hashTable_.resize(hashSize);

		for(const std::string& s : words)
		{
			insert(s);
		}
		size_ = words.size();
	}

	bool in_dictionary(string_view word) const
	{
		auto hash = std::hash<string_view>()(word);

		auto idx = index(hash);
		std::size_t attempts = 0;

		// 使用attempts是防止无限循环
		while (attempts < size_)
		{
			const Entry& entry = hashTable_[idx];

			if (!entry.string)
				return false;

			if(entry.hash == hash && *entry.string == word)
				return true;

			idx = next(idx);
			attempts++;
		}
		return false;
	}

private:
    // Linear probing:
    // !! 这段代码是精髓: 给予一个string s, 找到它所对应的value
    // 1. string_to_string_view: s -> sv
    // 2. calculate hash<string_view>()(sv); //计算hash值
    // 3. calculate idx = index(hash); // 通过hash计算存储数组的index
	// 		index的计算方法: return hash & (hashTable_.size()-1);
	// 		实际上是hash % hashTable_.size(). 但是如果hashTable.size()是2.^n, 
	// 		则上面的求模运算可以使用hash & (hashTable_.size() - 1)来代替, 效率会快很多)
    // 4. 根据idx来查找Entry: Entry& entry = hashTable_[idx];
	// 		4.1 如果entry已经被占用(if (entry.string) == true), 则查找下一个idx = next(idx).
	// 			计算方法如下: 
	// 			next = (idx + 1) & (hashTable_.size()-1);
	// 			回到4继续判断
	// 			
	// 		4.2 否则直接使用该空的entry并设置参数: 
	// 
	// 			entry.string = &s;
	// 			entry.hash = hash;			
	// 
	// 			跳出循环
	// 	
	// 
	// 
	void insert(const std::string& s)
	{
		string_view sv(s);
		auto hash = std::hash<string_view>()(sv);
		auto idx = index(hash);

		while(true)
		{
			Entry& entry = hashTable_[idx];
			if (entry.string)
			{
				idx = next(idx);
			}
			else
			{
				entry.string = &s;
				entry.hash = hash;
				break;
			}
		}
	}

	// & operation takes less cpu cycles than mod operation
	std::size_t index(std::size_t hash) const { return hash & (hashTable_.size()-1); }
	std::size_t next(std::size_t idx) const { return (idx + 1) & (hashTable_.size()-1); }

	struct Entry
	{
		std::size_t hash;
		const std::string* string = nullptr;
	};

	std::vector<Entry> hashTable_;
	std::size_t size_ = 0;
};

}
