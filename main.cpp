#include <iostream>
#include <vector>
#include <typeinfo>

struct obj_prefix {
	template<typename Ty>
	obj_prefix(Ty* ptr) {
		obj_size = sizeof(Ty);
		obj_occupied = sizeof(Ty);
		next = nullptr;
		//is_active = true;
	}
	obj_prefix* next;
	size_t obj_size;
	size_t obj_occupied;
	//bool is_active;

};

class allocator {
public:
	allocator(size_t block_size){
	
		m_block_begin = malloc(block_size);
		m_block_size = block_size;
		m_occupied = 0;
		m_prefix_size = sizeof(obj_prefix);
		m_first_free = nullptr;
		m_last_free = nullptr;

	}
	~allocator(){}
	template<typename Ty, typename...Args>
	Ty* emplace_back(Args&&...args) {
		char* place = static_cast<char*>(m_block_begin) + m_occupied;

		Ty* _new = new(place + m_prefix_size) Ty(std::forward<Args>(args)...);

		new(place) obj_prefix(_new);

		m_occupied += m_prefix_size + sizeof(Ty);

		return _new;
		
	}

	template<typename Ty, typename...Args>
	Ty* emplace(Args&&...args) {
		obj_prefix* _place = m_first_free;
		while (_place) {
			if (_place->obj_size == sizeof(Ty)) {
				return emplace_reuse<Ty>(_place, std::forward<Args>(args)...);
			}
			if (_place->obj_size >= sizeof(Ty) + m_prefix_size) {
				return emplace_at_free<Ty>(_place, std::forward<Args>(args)...);
			}
			_place = _place->next;
		}

		
		return emplace_back<Ty>(std::forward<Args>(args)...);

	}


	template<typename Ty, typename...Args>
	Ty* emplace_at_free(obj_prefix* ptr, Args&&...args) {
		char* place = reinterpret_cast<char*>(ptr) + ptr->obj_size - m_prefix_size - sizeof(Ty);

		Ty* _new = new(place) Ty(std::forward<Args>(args)...);

		new(place) obj_prefix(_new);

		ptr->obj_occupied = m_prefix_size + sizeof(Ty);

		return _new;
	}

	template<typename Ty, typename...Args>
	Ty* emplace_reuse(obj_prefix* ptr, Args&&...args) {
		char* place = reinterpret_cast<char*>(ptr) + m_prefix_size;

		Ty* _new = new(place + m_prefix_size) Ty(std::forward<Args>(args)...);

		ptr->obj_occupied = sizeof(Ty);

		return _new;
	}

	void remove(void* ptr) {

		obj_prefix* _prefix = reinterpret_cast<obj_prefix*>(static_cast<char*>(ptr) - m_prefix_size);
		_prefix->obj_occupied = 0;
		//_prefix->is_active = false;

		if (!m_last_free && !m_first_free) {
			m_first_free = _prefix;
			m_last_free = _prefix;
			std::cout << "new all" << '\n';

			return;
		}

		if (_prefix < m_first_free) {
			_prefix->next = m_first_free;
			m_first_free = _prefix;
			right_fold(_prefix);
			std::cout << "new first" << '\n';
			return;
		}

		if (_prefix > m_last_free) {
			m_last_free->next = _prefix;
			if (!right_fold(m_last_free)) {
				m_last_free = _prefix;
			}

			std::cout << "new last" << '\n';
			return;
		}

		insert_free(_prefix);

		std::cout << "new" << '\n';

	}


	void insert_free(obj_prefix* prefix) {
		obj_prefix* current_prefix = m_first_free;
		while (current_prefix->next < prefix) {
			current_prefix = current_prefix->next;
		}

		prefix->next = current_prefix->next;
		current_prefix->next = prefix;

		right_fold(current_prefix);
		right_fold(current_prefix);
	}

	//free is input [free | new free] -> [big free], returns if fold was succsessful
	bool right_fold(obj_prefix* prefix) {
		if (reinterpret_cast<char*>(prefix) + m_prefix_size + prefix->obj_size == reinterpret_cast<char*>(prefix->next)) {
			std::cout << "was " << prefix->obj_size << '\n';
			prefix->obj_size += m_prefix_size + prefix->next->obj_size;
			prefix->next = prefix->next->next;
			std::cout << "size is now " << prefix->obj_size << '\n';
			return true;
		}
		return false;

	}

	size_t block_size(void* ptr) {
		obj_prefix* _prefix = reinterpret_cast<obj_prefix*>(static_cast<char*>(ptr) - m_prefix_size);
		return _prefix->obj_size;
	}

private:
	void* m_block_begin;

	size_t m_occupied;
	size_t m_block_size;
	size_t m_prefix_size;

	obj_prefix* m_first_free;
	obj_prefix* m_last_free;

};

int main() {

	allocator alc(1024);

	auto x = alc.emplace<double>(14);
	auto y = alc.emplace<double>(14);
	//auto z = alc.emplace<double>(14);

	alc.remove(x);
	alc.remove(y);
	//alc.remove(z);
	auto z = alc.emplace<double>(14);


	std::cout << alc.block_size(x);

}