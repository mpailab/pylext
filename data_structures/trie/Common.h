inline int popcnt(uint64_t x) {
	return (int)_mm_popcnt_u64(x);
}

using namespace std;

template<class V, int max_size>
class Vertix_Const
{
private:
	static constexpr int SIZE_OF_VERTIX = (max_size + 63) / 64;
	std::array<uint64_t, SIZE_OF_VERTIX> mask;
	std::array<int, SIZE_OF_VERTIX> counts;
	//std::vector<V> values;
	//V _default;
	int BIT_SIZE = sizeof(uint64_t) * 8;
public:
	std::vector<V> values;
	Vertix_Const() { mask.fill(uint64_t(0)); }
	/*Vertix_Const(V def) { _default = def; mask.fill(uint64_t(0)); }
	Vertix_Const(std::vector<pair<int, V>> dict, V def) {
		_default = def;
		mask.fill(uint64_t(0));
		sort(dict.begin(), dict.end(), [](pair<int, V> x, pair<int, V> y) { return x.first < y.first; });
		for (int i = 0; i < dict.size(); i++) {
			int d;
			bool res = true;
			for (int j = 0; j < i; j++) {
				if (dict[j].first == dict[i].second) {
					res = false;
				}
			}
			if (res) {
				d = dict[i].first / BIT_SIZE;
				mask[d] = mask[d] | uint64_t(1) << (dict[i].first % BIT_SIZE);
				values.push_back(dict[i].second);
			}
		}
		int sum = 0;
		for (int i = 0; i < SIZE_OF_VERTIX; i++) {
			int x = popcnt(mask[i]);
			counts[i] = sum;
			sum += x;
		}
	}*/

	int hash(int n) {
		int cnts;
		uint64_t m, t;
		cnts = counts[n / BIT_SIZE];
		m = mask[n / BIT_SIZE];
		t = uint64_t(1) << (n % BIT_SIZE);
		if ((t & m) == 0)
			return -1;
		return cnts + popcnt(m & (t - 1));
	}

	V& get(int n) {
		int x = hash(n);
		if (x == -1)
			return values[0];
		else
			return values[x];
	}

	void add(int n, V value) {
		typename std::vector<V>::iterator it;
		int x = hash(n), d;
		if (x != -1) {
			values[x] = value;
		}
		else {
			d = n / BIT_SIZE;
			mask[d] = mask[d] | uint64_t(1) << (n % BIT_SIZE);
			int sum = 0;
			for (int i = 0; i < SIZE_OF_VERTIX; i++) {
				int x = popcnt(mask[i]);
				counts[i] = sum;
				sum += x;
			}
			values.insert(std::next(values.begin(), hash(n)), std::move(value));
		}

	}

	/*void del(int n) {
		typename std::vector<V>::iterator it;
		int x = hash(n), d;
		if (x != -1) {
			d = n / BIT_SIZE;
			mask[d] = mask[d] & ~(uint64_t(1) << (n % BIT_SIZE));
			int sum = 0;
			for (int i = 0; i < SIZE_OF_VERTIX; i++) {
				int x = popcnt(mask[i]);
				counts[i] = sum;
				sum += x;
			}
			int i = 0;
			int c = hash(n);
			for (it = values.begin(); it != values.end();) {
				if (i == c)
					break;
				i++;
			}
			values.erase(it);
		}
	}*/
	void clear(void) {
		mask.fill(uint64_t(0));
		values.clear();
		counts.fill(0);
	}

};