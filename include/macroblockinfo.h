#ifndef _MACROBLOCKINFO_H_
#define _MACROBLOCKINFO_H_
// StdLib
#include <inttypes.h>
#include <vector>
// NOTE: we could use MotionVector but it includes defines.h where
// ColorComponent is defined, thus leading to LNK2005 already defined
// error. The header should only declare ColorComponent to fix this issue.
// This is a basic workaround so that the JM library can be kept unchaged.
struct MotionVec {
	short mv_x;
	short mv_y;
};
// JM forward declaration
// typedef long long int64_t;

// structure to store relevant information (subset of Macroblock)
class MacroblockInfo {
	friend class Vis;
public:
	int pix_x;
	int pix_y;
	int qp;
	short mb_type;
	int slice_type;
	int64_t cbp_bits;
	std::vector<short> sub_mb_type;
	MotionVec get_mv(int blk_x, int blk_y, int list);
	int get_ref_idx(int blk_x, int blk_y, int list);
	static int get_size();
	void serialize(char* buffer);
	void deserialize(char* buffer);
private:
	std::vector<MotionVec> mv_;
	std::vector<int> ref_idx_;
};
#endif // _MACROBLOCKINFO_H_
