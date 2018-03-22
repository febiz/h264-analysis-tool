#include "vis.h"
// StdLib
#include <iostream>
#include <iomanip>
#include <sstream>
// CV
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
// zlib
#include "zlib_buffer.h"
// JM
#include "mbuffer.h"


// TODO(matt): figure out a robust way to get arrow keys
// Lenovo laptop
#define LEFT_ARROW_CODE 65361
#define RIGHT_ARROW_CODE 65363

// iMac
// #define RIGHT_ARROW_CODE 63235
// #define LEFT_ARROW_CODE 63234


bool Vis::play(bool dec) {
    cv::Mat frame;
    std::vector<MacroblockInfo>* mb_data;
    if (!dec) { // not decoding, might be requesting unavailable frame
        if (!get_mb_data(pos_, mb_data)) {
            pos_--;
        }
    }
    // check if last view avaiable
    int view_id = view_id_;
    view_id_ = frames_.size()-1;
    if (!get_frame(pos_, frame)) {
        view_id_ = view_id;
        return false; // fetch
    } else {
        view_id_ = view_id; // reset view if available
    }
    if (get_frame(pos_, frame)) {
        std::stringstream title_ss;
        cv::namedWindow("H.264", 1);
        cv::setMouseCallback("H.264", mouse_callback, this);
        while (true) {
            cv::Mat processed_frame = frame.clone();
            // TODO(matt): some videos have reversed R and B on mac and linux
            // cv::cvtColor(frame, processed_frame, CV_RGB2BGR);
            process_frame(processed_frame);
            cv::imshow("H.264", processed_frame);
            int key = cv::waitKeyEx(10);
            if (key != -1) {
              printf("got a key %d\n", key);
            }
            // if key event disable mb_info
            enabled_mb_info_ = key > 0 ? false : enabled_mb_info_;

            if (key == RIGHT_ARROW_CODE || key == '>' || key == '.') {
                // Detected right arrow: advance a frame.
                std::vector<MacroblockInfo>* mb_data;
                if (dec || get_mb_data(pos_+1, mb_data)) {
                    pos_++;
                    clear(pos_ - max_buffer_size_);
                }
                if (!get_frame(pos_, frame)) {
                    break;
                } else {
                    // frame not available, need to fetch
                }
            } else if (key == LEFT_ARROW_CODE || key == '<' || key == ',') {
                // Detected left arrow: go back a frame.
                if (pos_ > 0) {
                    if (get_frame(pos_-1, frame)) {
                        pos_--;
                        clear(pos_ + max_buffer_size_);
                    } else {
                        // frame not available, need to fetch
                    }
                }
            } else if (key == 'v') {
                view_id_ = (view_id_+1) % frames_.size();
                if (!get_frame(pos_, frame)) {
                    // fetch frame
                    break;
                }
            } else if (key == 'g') {
                enabled_mb_grid = enabled_mb_grid ? false : true;
            } else if (key == 'p') {
                enabled_mb_partitions = enabled_mb_partitions ? false : true;
            } else if (key == 'm') {
                enabled_mb_modes = enabled_mb_modes ? false : true;
            } else if (key == 'f') {
                enabled_mv_ = enabled_mv_ ? false : true;
            } else if (key == 'i') {
                enabled_frame_info_ = enabled_frame_info_ ? false : true;
            } else if (key == 'q') {
                wipe_buffers();
                return true;
            }
        }
    }
    return false;
}

void Vis::mouse_callback(int event, int x, int y, int flags, void* class_ptr) {
    Vis *self = static_cast<Vis*>(class_ptr);
    if (event == cv::EVENT_LBUTTONDOWN) {
        self->enabled_mb_info_ = true;
        self->mb_info_pt_ = cv::Point(x,y);
    }
}

bool Vis::get_frame(int pos, cv::Mat& next_frame) {
    if (frames_.empty()) {
        return false;
    }
    std::vector<std::pair<cv::Mat, int> >::iterator it = frames_[view_id_].end();
    while (it != frames_[view_id_].begin()) {
        it--;
        if (it->second < pos) {
            return false;
        } else if (it->second == pos) {
            if (it->first.empty()) {
                deserialize(pos);
            }
            next_frame = it->first;
            return true;
        }
    }
    return false;
}

bool Vis::get_mb_data(int pos, std::vector<MacroblockInfo>*& mb_data) {
    if (mb_data_.empty()) {
        return false;
    }
    std::vector<std::pair<std::vector<MacroblockInfo>, int> >::iterator it = mb_data_[view_id_].end();
    while (it != mb_data_[view_id_].begin()) {
        it--;
        if (it->second < pos) {
            return false;
        } else if (it->second == pos) {
            if (it->first.empty()) {
                deserialize(pos);
            }
            mb_data = &it->first;
            return true;
        }
    }
    return false;
}

void Vis::process_frame(cv::Mat& frame) {
    if (enabled_mb_grid) {
        draw_mb_grid(frame);
    }
    if (enabled_mb_partitions) {
        draw_mb_partitions(frame);
    }
    if (enabled_mb_modes) {
        draw_mb_mode(frame);
    }
    if (enabled_mv_) {
        draw_mvd(frame);
    }
    if (enabled_mb_info_) {
        draw_mb_info(frame);
    }
    if (enabled_frame_info_) {
        draw_frame_info(frame);
    }
}

void Vis::draw_mb_info(cv::Mat& frame) {
    double alpha = 0.75;
    cv::Mat frame_cpy = frame.clone();
    std::vector<MacroblockInfo>* mb_data;
    get_mb_data(pos_, mb_data);
    // mark selected macroblock
    mb_info_pt_.x = (mb_info_pt_.x/BLOCK_PIXELS)*BLOCK_PIXELS+BLOCK_PIXELS/2;
    mb_info_pt_.y = (mb_info_pt_.y/BLOCK_PIXELS)*BLOCK_PIXELS+BLOCK_PIXELS/2;
    int x_blk = mb_info_pt_.x/BLOCK_PIXELS;
    int y_blk = mb_info_pt_.y/BLOCK_PIXELS;
    int blk_pos = y_blk*frame.cols/BLOCK_PIXELS + x_blk;
    cv::rectangle(frame_cpy, cv::Point(x_blk*BLOCK_PIXELS, y_blk*BLOCK_PIXELS),
        cv::Point(x_blk*BLOCK_PIXELS+BLOCK_PIXELS, y_blk*BLOCK_PIXELS+BLOCK_PIXELS),
        cv::Scalar(255, 255, 255, 0), CV_FILLED);
    // position, mode and qp information
    std::vector<std::string> mb_info;
    {
        std::stringstream ss;
        ss << "--- MACROBLOCK INFORMATION ---";
        mb_info.push_back(ss.str());
    }
    {
        std::stringstream ss;
        ss << "Pos: x: " << (*mb_data)[blk_pos].pix_x/BLOCK_PIXELS << ", y: " <<
            (*mb_data)[blk_pos].pix_y/BLOCK_PIXELS;
        mb_info.push_back(ss.str());
    }
    {
        std::stringstream ss;
        ss << "Mode: " << num_to_mode((*mb_data)[blk_pos].mb_type);
        mb_info.push_back(ss.str());
    }
    {
        std::stringstream ss;
        ss << "QP: " << (*mb_data)[blk_pos].qp;
        mb_info.push_back(ss.str());
    }
    // motion vector information
    int endi = 1;
    int endj = 1;
    int stepi = 1;
    int stepj = 1;
    if ((*mb_data)[blk_pos].mb_type != P16x16 && (*mb_data)[blk_pos].mb_type != P16x8 &&
        (*mb_data)[blk_pos].mb_type != P8x16 && (*mb_data)[blk_pos].mb_type != P8x8 &&
        (*mb_data)[blk_pos].mb_type != PSKIP && (*mb_data)[blk_pos].mb_type != BSKIP_DIRECT) {
            endi = 0;
            endj = 0;
    }
    int xoff = BLOCK_SIZE_8x8;
    int yoff = BLOCK_SIZE_8x8;
    if ((*mb_data)[blk_pos].mb_type == P16x8 || (*mb_data)[blk_pos].mb_type == P8x8) {
        endj = 3;
        stepj = 2;
        yoff /= 2;
    }
    if ((*mb_data)[blk_pos].mb_type == P8x16 || (*mb_data)[blk_pos].mb_type == P8x8) {
        endi = 3;
        stepi = 2;
        xoff /= 2;
    }
    for (int j = 0; j < endj; j+=stepj) {
        for (int i = 0; i < endi; i+=stepi) {
            int stepii = stepi;
            int stepjj = stepj;
            int divxoff = 1;
            int divyoff = 1;
            if ((*mb_data)[blk_pos].mb_type == P8x8) {
                int sub_type = (*mb_data)[blk_pos].sub_mb_type[(j/2)*2+(i/2)];
                if (sub_type == SMB8x4 || sub_type == SMB4x4) {
                    stepjj /= 2;
                    divyoff = 2;
                }
                if (sub_type == SMB4x8 || sub_type == SMB4x4) {
                    stepii /= 2;
                    divxoff = 2;
                }
            }
            for (int jj = j; jj < j+stepj; jj+=stepjj) {
                for (int ii = i; ii < i+stepi; ii+=stepii) {
                    for (int l = 0; l <= LIST_1; ++l) {
                        if ((*mb_data)[blk_pos].get_ref_idx(ii, jj, l) != -1) {
                            MotionVec mv = (*mb_data)[blk_pos].get_mv(ii, jj, l);
                            double dx = mv.mv_x/4.; // NOTE: assuming QUARTER_PEL
                            double dy = mv.mv_y/4.; // NOTE: assuming QUARTER_PEL
                            std::stringstream ss;
                            ss << "MV (" << ii << ", " << jj << ", " << l << "): "
                                << (*mb_data)[blk_pos].get_mv(i, j, l).mv_x/4. << ", "
                                << (*mb_data)[blk_pos].get_mv(i, j, l).mv_y/4. << " "
                                << "(" << (*mb_data)[blk_pos].get_ref_idx(i, j, l) << ")";
                            mb_info.push_back(ss.str());
                        }
                    }
                }
            }
        }
    }
    // draw all info
    int width = 14+250;
    int height = 12*mb_info.size();
    cv::Point mb_info_offset(0, 0);
    // compute offset to ensure mb info is visible
    if (mb_info_pt_.y > frame.rows/2) {
        mb_info_offset.y = -height;
    }
    if (mb_info_pt_.x > frame.cols/2) {
        mb_info_offset.x = -width-14;
    }
    cv::rectangle(frame, mb_info_pt_+mb_info_offset+cv::Point(14, 0),
        mb_info_pt_+mb_info_offset+cv::Point(width, height),
        cv::Scalar(255, 255, 255, 0), CV_FILLED);
    std::vector<std::string>::iterator it = mb_info.begin();
    int count = 0;
    for (; it != mb_info.end(); ++it) {
        cv::putText(frame, *it, mb_info_pt_+mb_info_offset+cv::Point(16, 10+12*count++),
            cv::FONT_HERSHEY_SIMPLEX, 0.4f, cv::Scalar(0, 0, 0, 0), 1, CV_AA);
    }
    frame = frame*alpha + frame_cpy*(1.-alpha);
}

std::string Vis::num_to_mode(int mode) {
    if (mode == PSKIP || BSKIP_DIRECT) {
        return std::string("Skip/Direct");
    } else if (mode == I16MB) {
        return std::string("Intra 16x16");
    } else if (mode == I8MB) {
        return std::string("Intra 8x8");
    } else if (mode == I4MB) {
        return std::string("Intra 4x4");
    } else if (mode == P16x16) {
        return std::string("Inter 16x16");
    } else if (mode == P16x8) {
        return std::string("Inter 16x8");
    } else if (mode == P8x16) {
        return std::string("Inter 8x16");
    } else if (mode == P8x8) {
        return std::string("Inter 8x8");
    } else {
        return std::string("");
    }
}

void Vis::draw_mb_mode (cv::Mat& frame) {
    double alpha = 0.75;
    cv::Mat frame_cpy = frame.clone();
    std::vector<MacroblockInfo>* mb_data;
    get_mb_data(pos_, mb_data);
    std::vector<MacroblockInfo>::iterator it = mb_data->begin();
    for (; it != mb_data->end(); ++it) {
        cv::Scalar color;
        if (it->mb_type == I16MB || it->mb_type == I8MB || it->mb_type == I4MB || it->mb_type == IPCM) {
            color = cv::Scalar(0, 0, 255, 0);
        }
        if (it->mb_type == PSKIP || it->mb_type == BSKIP_DIRECT) {
            color = cv::Scalar(0, 255, 0, 0);
        }
        if (it->mb_type == P16x16 || it->mb_type == P16x8 || it->mb_type == P8x16 || it->mb_type == P8x8) {
            color = cv::Scalar(255, 0, 0, 0);
        }
        cv::rectangle(frame_cpy, cv::Point(it->pix_x, it->pix_y),
            cv::Point(it->pix_x+BLOCK_PIXELS, it->pix_y+BLOCK_PIXELS), color, CV_FILLED);
    }
    frame = frame*alpha + frame_cpy*(1.-alpha);
}

void Vis::draw_mb_grid (cv::Mat& frame) {
    for (int x = BLOCK_PIXELS; x < frame.cols; x += BLOCK_PIXELS) {
        cv::line(frame, cv::Point(x,0), cv::Point(x,frame.rows-1), cv::Scalar(0.f));
    }
    for (int y = BLOCK_PIXELS; y < frame.rows; y += BLOCK_PIXELS) {
        cv::line(frame, cv::Point(0,y), cv::Point(frame.cols-1,y), cv::Scalar(0.f));
    }
}

void Vis::draw_mb_partitions(cv::Mat& frame) {
    std::vector<MacroblockInfo>* mb_data;
    get_mb_data(pos_, mb_data);
    std::vector<MacroblockInfo>::iterator it = mb_data->begin();
    for (; it != mb_data->end(); ++it) {
        bool vhsplit = it->mb_type == I8MB || it->mb_type == P8x8 || it->mb_type == I4MB;
        if (it->mb_type == P16x8 || vhsplit) {
            draw_split(frame, it->pix_x, it->pix_y, BLOCK_PIXELS, HORIZONTAL);
        }
        if (it->mb_type == P8x16 || vhsplit) {
            draw_split(frame, it->pix_x, it->pix_y, BLOCK_PIXELS, VERTICAL);
        }
        // sub-partitions
        if (it->mb_type == I4MB || it->mb_type == P8x8) {
            bool split_all = it->mb_type == I4MB ? true : false;
            draw_split(frame, it->pix_x, it->pix_y, BLOCK_PIXELS, HORIZONTAL);
            draw_split(frame, it->pix_x, it->pix_y, BLOCK_PIXELS, VERTICAL);
            for (int b = 0; b < 4; ++b) {
                if (it->sub_mb_type[b] == SMB8x4 || it->sub_mb_type[b] == SMB4x4 || split_all) {
                    draw_split(frame, it->pix_x+(b%2)*BLOCK_SIZE_8x8, it->pix_y+(b/2)*BLOCK_SIZE_8x8,
                        BLOCK_SIZE_8x8, HORIZONTAL);
                }
                if (it->sub_mb_type[b] == SMB4x8 || it->sub_mb_type[b] == SMB4x4 || split_all) {
                    draw_split(frame, it->pix_x+(b%2)*BLOCK_SIZE_8x8, it->pix_y+(b/2)*BLOCK_SIZE_8x8,
                        BLOCK_SIZE_8x8, VERTICAL);
                }
            }
        }
    }
}

void Vis::draw_split(cv::Mat& frame, int xs, int ys, int len, direction dir) {
    if (dir == HORIZONTAL) {
        cv::line(frame, cv::Point(xs, ys+len/2), cv::Point(xs+len, ys+len/2), cv::Scalar(0.f));
    } else {
        cv::line(frame, cv::Point(xs+len/2, ys), cv::Point(xs+len/2, ys+len), cv::Scalar(0.f));
    }
}

void Vis::draw_mvd(cv::Mat& frame) {
    std::vector<MacroblockInfo>* mb_data;
    get_mb_data(pos_, mb_data);
    std::vector<MacroblockInfo>::iterator it = mb_data->begin();
    for (; it != mb_data->end(); ++it) {
        if (it->mb_type != P16x16 && it->mb_type != P16x8 && it->mb_type != P8x16 &&
            it->mb_type != P8x8 && it->mb_type != PSKIP && it->mb_type != BSKIP_DIRECT) {
                continue;
        }
        // default P16x16
        int endi = 1;
        int endj = 1;
        int stepi = 1;
        int stepj = 1;
        int xoff = BLOCK_SIZE_8x8;
        int yoff = BLOCK_SIZE_8x8;
        if (it->mb_type == P16x8 || it->mb_type == P8x8) {
            endj = 3;
            stepj = 2;
            yoff /= 2;
        }
        if (it->mb_type == P8x16 || it->mb_type == P8x8) {
            endi = 3;
            stepi = 2;
            xoff /= 2;
        }
        for (int i = 0; i < endi; i+=stepi) {
            for (int j = 0; j < endj; j+=stepj) {
                for (int l = 0; l <= LIST_1; ++l) {
                    int stepii = stepi;
                    int stepjj = stepj;
                    int divxoff = 1;
                    int divyoff = 1;
                    if (it->mb_type == P8x8) {
                        int sub_type = it->sub_mb_type[(j/2)*2+(i/2)];
                        if (sub_type == SMB8x4 || sub_type == SMB4x4) {
                            stepjj /= 2;
                            divyoff = 2;
                        }
                        if (sub_type == SMB4x8 || sub_type == SMB4x4) {
                            stepii /= 2;
                            divxoff = 2;
                        }
                    }
                    for (int ii = i; ii < i+stepi; ii+=stepii) {
                        for (int jj = j; jj < j+stepj; jj+=stepjj) {
                            MotionVec mv = it->get_mv(ii, jj, l);
                            double dx = mv.mv_x/4.; // NOTE: assuming QUARTER_PEL
                            double dy = mv.mv_y/4.; // NOTE: assuming QUARTER_PEL
                            double len = sqrt(dx*dx + dy*dy);
                            if (len > 0) {
                                double spinSize = 5.*std::min(len, 24.)/32.;
                                cv::Point p(it->pix_x+xoff/divxoff+ii*xoff, it->pix_y+yoff/divyoff+jj*yoff);
                                cv::Point p2(p.x + (int)(dx), p.y + (int)(dy));
                                cv::line(frame, p, p2, CV_RGB((1-l)*255, (1-l)*255, (1-l)*255), 1, CV_AA);
                                // arrow
                                double angle;
                                angle = atan2((double) p.y-p2.y, (double) p.x-p2.x);
                                p.x = (int) (p2.x + spinSize * cos(angle + 3.1416 / 4.));
                                p.y = (int) (p2.y + spinSize * sin(angle + 3.1416 / 4.));
                                cv::line(frame, p, p2, CV_RGB((1-l)*255, (1-l)*255, (1-l)*255), 1, CV_AA);
                                p.x = (int) (p2.x + spinSize * cos(angle - 3.1416 / 4.));
                                p.y = (int) (p2.y + spinSize * sin(angle - 3.1416 / 4.));
                                cv::line(frame, p, p2, CV_RGB((1-l)*255, (1-l)*255, (1-l)*255), 1, CV_AA);
                            }
                        }
                    }
                }
            }
        }
    }
}

void Vis::draw_frame_info(cv::Mat& frame) {
    std::vector<MacroblockInfo> *mb_data;
    get_mb_data(pos_, mb_data);
    // NOTE: assuming 1 slice
    char slice_type;
    if ((*mb_data)[0].slice_type == 0) {
        slice_type = 'P';
    } else if ((*mb_data)[0].slice_type == 1) {
        slice_type = 'B';
    } else if ((*mb_data)[0].slice_type == 2) {
        slice_type = 'I';
    } else {
        // TODO: should throw error instead
        slice_type = ' ';
    }
    double alpha = 0.75;
    cv::Mat frame_cpy = frame.clone();
    cv::rectangle(frame, cv::Point(2, 2), cv::Point(160, 20),
        cv::Scalar(255, 255, 255, 0), CV_FILLED);
    {
        std::stringstream ss;
        ss << std::setw(5) << std::setfill('0') << pos_
            << ", View: " << std::setw(1) << std::setfill('0') << view_id_
            << ", Type: " << slice_type;
        cv::putText(frame, ss.str(), cv::Point(2, 14),
            cv::FONT_HERSHEY_SIMPLEX, 0.4f, cv::Scalar(0, 0, 0, 0), 1, CV_AA);
    }
    frame = frame*alpha + frame_cpy*(1.-alpha);
}

void Vis::clear(int pos) {
    if (pos >= 0 && unsigned(pos) < frames_.back().size()) {
        frames_[0][pos].first.release();
        frames_[1][pos].first.release();
        mb_data_[0][pos].first.clear();
        mb_data_[1][pos].first.clear();
    }
}

void Vis::put(StorablePicture* spic, Macroblock *mb_data, int frame_num) {
    cv::Mat frame;
    storablepic_to_mat(spic, frame);
    if (frames_.empty() || frames_.size() < unsigned(spic->view_id+1)) {
        std::vector<std::pair<cv::Mat, int> > frames;
        frames.push_back(std::make_pair(frame, frame_num+offset_));
        frames_.push_back(frames);
    } else {
        std::vector<std::pair<cv::Mat, int> >::iterator it = frames_[spic->view_id].end();
        while(it != frames_[spic->view_id].begin()) {
            it--;
            if (it->second < frame_num+offset_) {
                frames_[spic->view_id].insert(it+1, 1, std::make_pair(frame, frame_num+offset_));
                break;
            } else if (it->second == frame_num+offset_) {
                // new gop, update offset and insert new frame
                it = frames_[spic->view_id].end()-1;
                offset_ = it->second-frame_num+1;
                frames_[spic->view_id].insert(it+1, 1, std::make_pair(frame, frame_num+offset_));
                break;
            }
        }
    }
    std::vector<MacroblockInfo> curr_mbs;
    curr_mbs.reserve(mb_data->p_Vid->FrameSizeInMbs);
    for (unsigned int k = 0; k < mb_data->p_Vid->FrameSizeInMbs; ++k) {
        MacroblockInfo mbi;
        mbi.pix_x = mb_data[k].pix_x;
        mbi.pix_y = mb_data[k].pix_y;
        mbi.qp = mb_data[k].qp;
        mbi.mb_type = mb_data[k].mb_type;
        mbi.slice_type = spic->slice_type;
        mbi.cbp_bits = mb_data[k].s_cbp->bits;
        mbi.sub_mb_type.reserve(BLOCK_MULTIPLE);
        for (int i = 0; i < BLOCK_MULTIPLE; ++i) {
            mbi.sub_mb_type.push_back(mb_data[k].b8mode[i]);
        }
        // mv and refs
        mbi.mv_.reserve(BLOCK_MULTIPLE*BLOCK_MULTIPLE*2);
        mbi.ref_idx_.reserve(BLOCK_MULTIPLE*BLOCK_MULTIPLE*2);
        for (int l = 0; l <= LIST_1; ++l) {
            for (int i = 0; i < BLOCK_MULTIPLE; ++i) {
                for (int j = 0; j < BLOCK_MULTIPLE; ++j) {
                    // NOTE: see macroblockinfo.h for details about this conversion
                    MotionVector mv = spic->mv_info[mb_data[k].block_y+j][mb_data[k].block_x+i].mv[l];
                    MotionVec mv2;
                    mv2.mv_x = mv.mv_x;
                    mv2.mv_y = mv.mv_y;
                    mbi.mv_.push_back(mv2);
                    if (mb_data[k].b8pdir[(j/2)*2+i/2] == l || mb_data[k].b8pdir[(j/2)*2+i/2] == 2) {
                        if (spic->mv_info[mb_data[k].block_y+j][mb_data[k].block_x+i].ref_idx[l] != -1) {
                            mbi.ref_idx_.push_back(spic->mv_info[mb_data[k].block_y+j][mb_data[k].block_x+i].ref_pic[l]->poc/2+offset_);
                        } else {
                            mbi.ref_idx_.push_back(-1);
                        }
                    } else {
                        mbi.ref_idx_.push_back(-1);
                    }
                }
            }
        }
        curr_mbs.push_back(mbi);
    }
    if (mb_data_.empty() || mb_data_.size() < unsigned(spic->view_id+1)) {
        std::vector<std::pair<std::vector<MacroblockInfo>, int> > all_mbs;
        all_mbs.push_back(std::make_pair(curr_mbs, frame_num+offset_));
        mb_data_.push_back(all_mbs);
        // will be called num_views times
        serialize(0);
    } else {
        int pos = -1;
        std::vector<std::pair<std::vector<MacroblockInfo>, int> >::iterator it = mb_data_[spic->view_id].end();
        while (it != mb_data_[spic->view_id].begin()) {
            it--;
            if (it->second < frame_num+offset_) {
                pos = int((it+1)-mb_data_[spic->view_id].begin());
                mb_data_[spic->view_id].insert(it+1, 1, std::make_pair(curr_mbs, frame_num+offset_));
                break;
            } else if (it->second == frame_num+offset_) {
                pos = int((it+1)-mb_data_[spic->view_id].begin());
                // new gop, update offset and insert new frame
                it = mb_data_[spic->view_id].end()-1;
                offset_ = it->second-frame_num+1;
                mb_data_[spic->view_id].insert(it+1, 1, std::make_pair(curr_mbs, frame_num+offset_));
                break;
            }
        }
        if (spic->view_id == mb_data_.size()-1) {
            serialize(pos);
        }
    }
}

void Vis::storablepic_to_mat(StorablePicture *spic, cv::Mat& cvpic) {
    if (!spic->frame_mbs_only_flag) {
    }
    if (spic->chroma_format_idc != YUV420) {
    }
    cvpic = cv::Mat(spic->size_y, spic->size_x, CV_8UC3);
    for (int y = 0; y < spic->size_y; ++y) {
        cv::Vec3b* ptr = cvpic.ptr<cv::Vec3b>(y);
        for (int x = 0; x < spic->size_x; ++x) {
            ptr[x][0] = (uchar) spic->imgY[y][x];           // Y
            ptr[x][1] = (uchar) spic->imgUV[0][y/2][x/2];   // U
            ptr[x][2] = (uchar) spic->imgUV[1][y/2][x/2];   // V
        }
    }
    cv::cvtColor(cvpic, cvpic, CV_YUV2BGR);
}

void Vis::serialize(int pos) {
    const int num_views = mb_data_.size();
    const int mbi_count = mb_data_[0][pos].first.size();
    const int mbi_size = MacroblockInfo::get_size();
    // TODO: allocate only once
    char* buffer = new char[num_views*mbi_count*mbi_size];
    std::stringstream mbss;
    mbss << "mb_data_"<< std::setw(6) << std::setfill('0') << frames_[0][pos].second << ".bin";
    // uncompressed: std::ofstream mb_file;
    // uncompressed: mb_file.open(mbss.str(), std::ios::binary);
    // NOTE: using C-style here due to zlib
    FILE *mb_file = fopen(mbss.str().c_str(), "wb");
    if (mb_file == NULL) {
        std::cout << "TODO: error" << std::endl;
    }
    int count = 0;
    for (unsigned int v = 0; v < frames_.size(); ++v) {
        std::stringstream fss;
        fss << "frame_" << std::setw(6) << std::setfill('0') << mb_data_[0][pos].second << "_"
            << std::setw(2) << std::setfill('0') << v << ".png";
        cv::imwrite(fss.str(), frames_[v][pos].first);
        // Macroblock information
        std::vector<MacroblockInfo>::iterator it = mb_data_[v][pos].first.begin();
        for (; it != mb_data_[v][pos].first.end(); ++it) {
            it->serialize(buffer+mbi_size*count++);
        }
    }
    // uncompressed: mb_file.write(buffer, MacroblockInfo::get_size());
    // uncompressed: mb_file.close();
#ifdef USE_ZLIB
    int ret = Zlib::def(buffer, num_views*mbi_size*mbi_count, mb_file, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK)
        return Zlib::zerr(ret);
#else
    fwrite(buffer, sizeof(char), num_views*mbi_size*mbi_count, mb_file);
#endif
    fclose(mb_file);
    delete [] buffer;
}

void Vis::deserialize(int pos) {
    for (unsigned int v = 0; v < frames_.size(); ++v) {
        std::stringstream fss;
        fss << "frame_" << std::setw(6) << std::setfill('0') << pos << "_"
            << std::setw(2) << std::setfill('0') << v << ".png";
        cv::Mat raw_input = cv::imread(fss.str());
        std::cout << "loading " << fss.str() << "\n";
        cv::cvtColor(raw_input, frames_[v][pos].first, CV_RGB2BGR);
    }
    const int num_views = mb_data_.size();
    const int mbi_count = frames_[0][pos].first.rows*frames_[0][pos].first.cols/BLOCK_PIXELS/BLOCK_PIXELS;
    const int mbi_size = MacroblockInfo::get_size();
    // TODO: allocate only once
    char* buffer = new char[num_views*mbi_count*mbi_size];
    std::stringstream mbss;
    mbss << "mb_data_"<< std::setw(6) << std::setfill('0') << pos << ".bin";
    // NOTE: using C-style here due to zlib
    FILE *mb_file = fopen(mbss.str().c_str(), "rb");
#ifdef USE_ZLIB
    int ret = Zlib::inf(mb_file, buffer, num_views*mbi_size*mbi_count);
    if (ret != Z_OK)
        return Zlib::zerr(ret);
#else
    fread(buffer, sizeof(char), num_views*mbi_size*mbi_count, mb_file);
#endif
    // uncompressed: std::ifstream mb_file;
    // uncompressed: mb_file._open(mbss.str(), std::ios::binary);
    int count = 0;
    for (unsigned int v = 0; v < frames_.size(); ++v) {
        std::vector<MacroblockInfo> mb_info_vec;
        for (int i = 0; i < mbi_count; ++i) {
            MacroblockInfo mb_info;
            //mb_file.read(buffer, MacroblockInfo::get_size());
            mb_info.deserialize(buffer+mbi_size*count++);
            mb_info_vec.push_back(mb_info);
        }
        mb_data_[v][pos].first = mb_info_vec;
    }
    //mb_file.close();
    fclose(mb_file);
    delete [] buffer;
}

void Vis::wipe_buffers()
{
    for (unsigned int v = 0; v < frames_.size(); ++v) {
        std::vector<std::pair<cv::Mat, int> >::iterator it = frames_[v].begin();
        for (; it != frames_[v].end(); ++it) {
            std::stringstream fss;
            fss << "frame_" << std::setw(6) << std::setfill('0') << it->second << "_"
                << std::setw(2) << std::setfill('0') << v << ".png";
            remove(fss.str().c_str());
            if (v == 0) {
                std::stringstream mbss;
                mbss << "mb_data_"<< std::setw(6) << std::setfill('0') << it->second << ".bin";
                remove(mbss.str().c_str());
            }
        }
    }
}
