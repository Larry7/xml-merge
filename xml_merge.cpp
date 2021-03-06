#include <fstream>
#include <vector>
#include <unordered_map>

namespace xml_merge {
	struct kstruct_t {
		std::string k;
		std::string v;
	};

	std::string load(const char* filename) {
		std::vector<char> file;
		if(std::ifstream is { filename, is.binary | is.ate }) {
			file.resize(static_cast<size_t>(is.tellg()));
			is.seekg(0);
			is.read(file.data(), file.size());
			file.push_back(0);
		}
		return file.data();
	}

	bool fclear(const char* filename) {
		if (std::ofstream os { filename, os.binary | os.ate })
			return true;
		return false;
	}

	void fout(const char* filename, const std::string& data) {
		if(std::ofstream os { filename, os.binary | os.ate | os.app}) {
			os << data.c_str();
		}
	}

	bool is_key_eq(const std::string& k1, const std::string& k2) {
		auto kk1 = k1, kk2 = k2;
		while (!kk1.empty() && isspace(kk1.back())) kk1.pop_back();
		while (!kk2.empty() && isspace(kk2.back())) kk2.pop_back();

		return kk1.compare(kk2) == 0;
	}

	void add_v(std::string& src, std::vector<kstruct_t>& sm, std::unordered_map<std::string, std::string>& du) {
		int s = src.find("<XmlTagLevelOne");
		while (s != std::string::npos)
		{
			auto before = src.substr(0, s);
			auto sub = src.substr(s);
			int e = sub.find("/XmlTagLevelOne>");
			if (e != std::string::npos) {
				kstruct_t pa;
				{
					auto cmp = before.compare(0, 10, "<XmlTagDoc");
					if (cmp == 0) {
						auto seqe = src.find_first_of('>');
						before = src.substr(seqe+1, s-seqe-1);
					}

					auto sub_key = sub.substr(10, sub.find_first_of('>')-10);
					auto ks = sub_key.find_first_of('"');

					pa.k = sub_key.substr(ks, sub_key.substr(ks+1).find_first_of('"')+2);
					pa.v = before + sub.substr(0, e+10);
				}
				
				for(const auto& k : sm) {
					if (is_key_eq(k.k, pa.k)) {
						du[pa.k] = pa.v;
					}
				}

				if (du.size() == 0) {
					sm.push_back(pa);
				} 
				src = src.substr(s+e+10);
			}
			s = src.find("<XmlTagLevelOne");
		}
	}

	void save(const char* des, const std::vector<kstruct_t>& dm) {
		fclear(des);
		for (const auto& d : dm) {
			fout(des, d.v);
		}
		fout(des, "\n</XmlTagDoc>\n");	
	}
};

int main(int argc, char* argv[])
{
	auto err = false;
	const auto size = argc-1;
	std::vector< std::vector<xml_merge::kstruct_t> > sm(size);
	const auto args = std::vector<std::string>(argv+1, argv+argc);
	for(auto i = 0; i < size; ++ i) {
		std::unordered_map<std::string, std::string> om;
		auto s = xml_merge::load(args[i].c_str());
		xml_merge::add_v(s, sm[i], om);
		if (om.size() > 0) {
			for (const auto& o : om) {
				printf ("*** duplicated workflow %s in %s ***\n", o.first.data(), args[i].c_str());
			}
		}

		if (!err) {
			err = om.size() > 0;
		}
	}

	if (err) {
		std::abort(); 
	}

	for(auto i = 0; i < size-1; ++ i) {
		for (const auto& ks : sm[i]) {
			auto rp = false;
			for (auto &ds : sm[size-1]) {
				if (xml_merge::is_key_eq(ks.k, ds.k)) {
					rp = true;
					ds.v = ks.v;
					break;
				}
			}
			if (!rp) {
				sm[size-1].push_back(ks);
			}
		}		
	}

	xml_merge::save(args[size-1].data(), sm[size-1]);
	
	printf("*** merged to %s done! ***\n", args[size-1].data());
}
