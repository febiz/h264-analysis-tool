#include "macroblockinfo.h"
#include <cstring>

MotionVec MacroblockInfo::get_mv(int blk_x, int blk_y, int list) {
	return mv_[list*16 + blk_y*4 + blk_x];
}

int MacroblockInfo::get_ref_idx(int blk_x, int blk_y, int list) {
	return ref_idx_[list*16 + blk_y*4 + blk_x];
}

int MacroblockInfo::get_size() {
	return 3*sizeof(int) + sizeof(short) + sizeof(int) + sizeof(long long)
		+ 4*sizeof(short) + 32*2*sizeof(short) + 32*sizeof(int);
}

void MacroblockInfo::serialize(char* buffer) {
    std::memcpy(buffer, &pix_x, sizeof(int));
	buffer += sizeof(int);
    std::memcpy(buffer, &pix_y, sizeof(int));
	buffer += sizeof(int);
    std::memcpy(buffer, &qp, sizeof(int));
	buffer += sizeof(int);
    std::memcpy(buffer, &mb_type, sizeof(short));
	buffer += sizeof(short);
    std::memcpy(buffer, &slice_type, sizeof(short));
	buffer += sizeof(short);
    std::memcpy(buffer, &cbp_bits, sizeof(long long));
	buffer += sizeof(long long);
	std::vector<short>::iterator stit = sub_mb_type.begin();
	for (; stit != sub_mb_type.end(); ++stit) {
        std::memcpy(buffer, &(*stit), sizeof(short));
		buffer += sizeof(short);
	}
	std::vector<MotionVec>::iterator mvit = mv_.begin();
	for(; mvit != mv_.end(); ++mvit) {
        std::memcpy(buffer, &(mvit->mv_x), sizeof(short));
		buffer += sizeof(short);
        std::memcpy(buffer, &(mvit->mv_y), sizeof(short));
		buffer += sizeof(short);
	}
	std::vector<int>::iterator refit = ref_idx_.begin();
	for(; refit != ref_idx_.end(); ++refit) {
        std::memcpy(buffer, &(*refit), sizeof(int));
		buffer += sizeof(int);
	}
}

void MacroblockInfo::deserialize(char* buffer) {
    std::memcpy(&pix_x, buffer, sizeof(int));
	buffer += sizeof(int);
    std::memcpy(&pix_y, buffer, sizeof(int));
	buffer += sizeof(int);
    std::memcpy(&qp, buffer, sizeof(int));
	buffer += sizeof(int);
    std::memcpy(&mb_type, buffer, sizeof(short));
	buffer += sizeof(short);
    std::memcpy(&slice_type, buffer, sizeof(short));
	buffer += sizeof(short);
    std::memcpy(&cbp_bits, buffer, sizeof(long long));
	buffer += sizeof(long long);
	for (int s = 0; s < 4; ++s) {
		short sub_type;
        std::memcpy(&sub_type, buffer, sizeof(short));
		buffer += sizeof(short);
		sub_mb_type.push_back(sub_type);
	}
	for (int m = 0; m < 32; ++m) {
		MotionVec mv;
        std::memcpy(&(mv.mv_x), buffer, sizeof(short));
		buffer += sizeof(short);
        std::memcpy(&(mv.mv_y), buffer, sizeof(short));
		buffer += sizeof(short);
		mv_.push_back(mv);
	}
	for (int r = 0; r < 32; ++r) {
		int ref;
        std::memcpy(&ref, buffer, sizeof(int));
		buffer += sizeof(int);
		ref_idx_.push_back(ref);
	}
}
