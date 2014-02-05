#include "macroblockinfo.h"

MotionVec MacroblockInfo::get_mv(int blk_x, int blk_y, int list) {
	return mv_[list*16 + blk_y*4 + blk_x];
}

int MacroblockInfo::get_ref_idx(int blk_x, int blk_y, int list) {
	return ref_idx_[list*16 + blk_y*4 + blk_x];
}

int MacroblockInfo::get_size() {
	return 3*sizeof(int) + sizeof(short) + sizeof(int) + sizeof(int64)
		+ 4*sizeof(short) + 32*2*sizeof(short) + 32*sizeof(int);
}

void MacroblockInfo::serialize(char* buffer) {
	memcpy(buffer, &pix_x, sizeof(int));
	buffer += sizeof(int);
	memcpy(buffer, &pix_y, sizeof(int));
	buffer += sizeof(int);
	memcpy(buffer, &qp, sizeof(int));
	buffer += sizeof(int);
	memcpy(buffer, &mb_type, sizeof(short));
	buffer += sizeof(short);
	memcpy(buffer, &slice_type, sizeof(short));
	buffer += sizeof(short);
	memcpy(buffer, &cbp_bits, sizeof(int64));
	buffer += sizeof(int64);
	std::vector<short>::iterator stit = sub_mb_type.begin();
	for (; stit != sub_mb_type.end(); ++stit) {
		memcpy(buffer, &(*stit), sizeof(short));
		buffer += sizeof(short);
	}
	std::vector<MotionVec>::iterator mvit = mv_.begin();
	for(; mvit != mv_.end(); ++mvit) {
		memcpy(buffer, &(mvit->mv_x), sizeof(short));
		buffer += sizeof(short);
		memcpy(buffer, &(mvit->mv_y), sizeof(short));
		buffer += sizeof(short);
	}
	std::vector<int>::iterator refit = ref_idx_.begin();
	for(; refit != ref_idx_.end(); ++refit) {
		memcpy(buffer, &(*refit), sizeof(int));
		buffer += sizeof(int);
	}
}

void MacroblockInfo::deserialize(char* buffer) {
	memcpy(&pix_x, buffer, sizeof(int));
	buffer += sizeof(int);
	memcpy(&pix_y, buffer, sizeof(int));
	buffer += sizeof(int);
	memcpy(&qp, buffer, sizeof(int));
	buffer += sizeof(int);
	memcpy(&mb_type, buffer, sizeof(short));
	buffer += sizeof(short);
	memcpy(&slice_type, buffer, sizeof(short));
	buffer += sizeof(short);
	memcpy(&cbp_bits, buffer, sizeof(int64));
	buffer += sizeof(int64);
	for (int s = 0; s < 4; ++s) {
		short sub_type;
		memcpy(&sub_type, buffer, sizeof(short));
		buffer += sizeof(short);
		sub_mb_type.push_back(sub_type);
	}
	for (int m = 0; m < 32; ++m) {
		MotionVec mv;
		memcpy(&(mv.mv_x), buffer, sizeof(short));
		buffer += sizeof(short);
		memcpy(&(mv.mv_y), buffer, sizeof(short));
		buffer += sizeof(short);
		mv_.push_back(mv);
	}
	for (int r = 0; r < 32; ++r) {
		int ref;
		memcpy(&ref, buffer, sizeof(int));
		buffer += sizeof(int);
		ref_idx_.push_back(ref);
	}
}