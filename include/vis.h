#ifndef _VIS_H_
#define _VIS_H_
// StdLib
#include <vector>
#include <utility>
// CV
#include "opencv2/core/core.hpp"
// H264 AT
#include "macroblockinfo.h"

// JM forward declarations
struct  storable_picture;
typedef struct storable_picture StorablePicture;
struct macroblock_dec;
typedef struct macroblock_dec Macroblock;

// MB split direction
enum direction {
	HORIZONTAL,
	VERTICAL
};

class Vis {
public:
	Vis() : pos_(0), offset_(0), view_id_(0), enabled_mb_grid(true), enabled_mb_partitions(true),
		enabled_mb_modes(true), enabled_mv_(true), enabled_mb_info_(false), enabled_frame_info_(true)
	{
	};
	~Vis() {
		wipe_buffers();
	}
	// display ----------------------------------------------------------------
	bool play(bool dec);
	static void mouse_callback(int event, int x, int y, int flags, void* class_ptr);
	// fetch data -------------------------------------------------------------
	bool get_frame(int pos, cv::Mat& next_frame);
	bool get_mb_data(int pos, std::vector<MacroblockInfo>*& mb_data);
	// draw metadata related --------------------------------------------------
	void process_frame(cv::Mat& frame);
	void draw_mb_info(cv::Mat& frame);
	std::string num_to_mode(int mode);
	// mode related -----------------------------------------------------------
	void draw_mb_mode (cv::Mat& frame);
	// partition related ------------------------------------------------------
	void draw_mb_grid (cv::Mat& frame);
	void draw_mb_partitions(cv::Mat& frame);
	void draw_split(cv::Mat& frame, int xs, int ys, int len, direction dir);
	// motion vector related --------------------------------------------------
	void draw_mvd(cv::Mat& frame);
	// frame info related -----------------------------------------------------
	void draw_frame_info(cv::Mat& frame);
	// metadata buffer management ---------------------------------------------
	void clear(int pos);
	void put(StorablePicture* spic, Macroblock *mb_data, int frame_num);
	// convert storable picture to cv::Mat ------------------------------------
	void storablepic_to_mat(StorablePicture *spic, cv::Mat& cvpic);
	// serialization ----------------------------------------------------------
	void serialize(int pos);
	void deserialize(int pos);
	void wipe_buffers();
private:
	int pos_;
	int offset_;
	int view_id_;
	// members related to partition
	bool enabled_mb_grid;
	bool enabled_mb_partitions;
	// members related to modes
	bool enabled_mb_modes;
	// members related to mv
	bool enabled_mv_;
	// members related to metadata
	bool enabled_mb_info_;
	cv::Point mb_info_pt_;
	// members related to frame info
	bool enabled_frame_info_;
	// buffers and metadata
	std::vector<std::vector<std::pair<cv::Mat, int> > > frames_;
	std::vector<std::vector<std::pair<std::vector<MacroblockInfo>, int> > > mb_data_;
	static const int max_buffer_size_ = 60;
};
#endif // _VIS_H_