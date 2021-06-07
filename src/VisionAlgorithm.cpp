#include "Main.hpp"

int coffset [] = {
    0,
    128,
    142,
    133,
    113,
    144,
    108, //114
    120,
    128,
    110
};

cv::Mat findAngles(cv::Mat image, float binning){
    cv::Mat sobelx, sobely, magnitudes, angles;

    cv::Sobel(image, sobelx, CV_32F, 1, 0, 1);
    cv::Sobel(image, sobely, CV_32F, 0, 1, 1);
    cv::cartToPolar(sobelx, sobely, magnitudes, angles, true);

    cv::resize(angles, angles, cv::Size(), binning, binning, cv::INTER_AREA );

    return angles;
}


double val_min, val_max;
cv::Point getPosition(cv::Mat &search){
    cv::Point loc_min, loc_max;
    cv::minMaxLoc(search, &val_min, &val_max, &loc_min, &loc_max);
    return loc_max;
}

bool confidence(Images *images, cv::Mat&result){
    float scale_factor = 0.5;
    cv::Mat heat_graph, image, templat;
    cv::resize( images->current_image, image, cv::Size(), scale_factor, scale_factor );
    cv::resize( images->previous_image, templat, cv::Size(), scale_factor, scale_factor );
    cv::matchTemplate(image, templat, heat_graph, cv::TM_CCOEFF_NORMED);
    float similarity = heat_graph.at<float>(0,0);

    int rows = result.rows;
    cv::normalize(result, result, 0, 1, cv::NORM_MINMAX);
    cv::reduce(result, result, 0, cv::REDUCE_SUM);
    float noise = result.at<float>(0,0)/rows;
    noise = 1/(noise*10);

    float similarity_weight = 0.5;
    float noise_weight = 1;

    float confidence_value = (similarity*similarity_weight)+(noise*noise_weight);

    //std::cout << std::fixed;
    //std::cout << std::setprecision(2);
    //std::cout << "\t" << val_max << "\t" << similarity << "\t" << noise << "\t" << confidence_value << std::endl;

    // tufted: >0.4 >0.7

    if (images->program == 1)
        return (confidence_value>0.2);
    else
        return (confidence_value>0.61);
}

void computeSpeed(Images *images){
    cv::Mat image, templat, heat_map;
    float scale_factor = 0.5;
    int margin_y = 60;

    cv::Rect i_roi(0, margin_y,images->current_image.cols,images->current_image.rows-(margin_y*2));
    cv::Rect p_roi(6,6,4,130);
    
    cv::resize( images->current_image(i_roi), image, cv::Size(), scale_factor, scale_factor );
    cv::resize( (images->previous_image(i_roi))(p_roi), templat, cv::Size(), scale_factor, scale_factor );
    
    cv::matchTemplate(image, templat, heat_map, cv::TM_CCOEFF_NORMED);
    
    cv::Point position = getPosition(heat_map);

    int travel = (position.x/scale_factor) - 6;
    images->travel = travel*4;
}

int stack_head = 0;
int stack_base = 7;
std::vector<cv::Mat> image_stack(stack_base);
cv::Mat stacked = cv::Mat::zeros(256, 320, CV_8UC3);
bool full_stack = false;

void computeMovement(Images *images){
    computeSpeed(images);

    cv::Mat angles, result, result1, result2, im, tm, sobely, sobelx, magnitudes;

    int center_cam = coffset[images->module_number];
    int window_offset = 0;
    cv::Point position;
    int shift = 0;

    //images->program = 4;

    if (images->program == 1){
        images->shift_average.base = 7;
        cv::Rect roi( 0, (center_cam)-25, images->pattern_image.cols, 50);
        cv::resize(images->current_image, im, cv::Size(), 0.5, 1, cv::INTER_LINEAR );
        cv::resize(images->pattern_image(roi), tm, cv::Size(), 0.5, 1, cv::INTER_LINEAR );
        cv::matchTemplate(im, tm, result, cv::TM_CCOEFF_NORMED);
        position = getPosition(result);
        window_offset = 25;
        shift = (position.y+window_offset);
        shift = -((shift-center_cam)*4);
        //std::cout << "Tufted" << std::endl;
    }
    else if (images->program == 4){
        cv::cvtColor(images->current_image, im, cv::COLOR_BGR2GRAY);
        angles = findAngles(im);
        cv::Rect roi( 0, (center_cam/2)-20, images->pattern_angles.cols, 40);
        cv::matchTemplate(angles, images->pattern_angles(roi), result, cv::TM_CCOEFF_NORMED);
        position = getPosition(result);
        window_offset = 20;
        shift = (position.y+window_offset)*2;
        shift = -((shift-(center_cam))*4);
        //std::cout << "fuzzy tufted" << std::endl;
    }
    else if (images->program == 2){
        images->shift_average.base = 3;

        cv::Mat bgr[3];   //destination array
        cv::split(images->current_image,bgr);//split source  

        cv::Mat Rx,Ry,Rm,Ra,Rs;
        cv::Mat Gx,Gy,Gm,Ga,Gs;
        cv::Mat Bx,By,Bm,Ba,Bs;

        cv::Sobel(bgr[0], Rx, CV_32F, 1, 0, 1);
        cv::Sobel(bgr[0], Ry, CV_32F, 0, 1, 1);
        cv::cartToPolar(Rx, Ry, Rm, Ra, true);
        cv::resize(Rx, Rx, cv::Size(128,128), 0,0, cv::INTER_AREA );
        cv::resize(Ry, Ry, cv::Size(128,128), 0,0, cv::INTER_AREA );
        cv::resize(Rm, Rm, cv::Size(128,128), 0,0, cv::INTER_AREA );

        cv::Sobel(bgr[1], Gx, CV_32F, 1, 0, 1);
        cv::Sobel(bgr[1], Gy, CV_32F, 0, 1, 1);
        cv::cartToPolar(Gx, Gy, Gm, Ga, true);
        cv::resize(Gx, Gx, cv::Size(128,128), 0,0, cv::INTER_AREA );
        cv::resize(Gy, Gy, cv::Size(128,128), 0,0, cv::INTER_AREA );
        cv::resize(Gm, Gm, cv::Size(128,128), 0,0, cv::INTER_AREA );

        cv::Sobel(bgr[2], Bx, CV_32F, 1, 0, 1);
        cv::Sobel(bgr[2], By, CV_32F, 0, 1, 1);
        cv::cartToPolar(Bx, By, Bm, Ba, true);
        cv::resize(Bx, Bx, cv::Size(128,128), 0,0, cv::INTER_AREA );
        cv::resize(By, By, cv::Size(128,128), 0,0, cv::INTER_AREA );
        cv::resize(Bm, Bm, cv::Size(128,128), 0,0, cv::INTER_AREA );

        Rs = Ry-Rx;
        cv::normalize(Rs,Rs, 0,255, cv::NORM_MINMAX);
        Rs = Rs + Rm;
        cv::reduce(Rs, Rs, 1, cv::REDUCE_SUM);

        Gs = Gy-Gx;
        cv::normalize(Gs,Gs, 0,255, cv::NORM_MINMAX);
        Gs = Gs + Gm;
        cv::reduce(Gs, Gs, 1, cv::REDUCE_SUM);
        
        Bs = By-Bx;
        cv::normalize(Bs,Bs, 0,255, cv::NORM_MINMAX);
        Bs = Bs + Bm;
        cv::reduce(Bs, Bs, 1, cv::REDUCE_SUM);

        cv::Point position1, position2, position3;

        position1 = getPosition(Rs);
        position2 = getPosition(Gs);
        position3 = getPosition(Bs);

        shift = ((position1.y+position2.y+position3.y)/3)*2;
        //std::cout << "\t" << shift << "\t";
        shift = -((shift-(center_cam))*4);
        images->shift = shift;
    }
    else if (images->program == 5){
        images->shift_average.base = 3;
        if (full_stack)
            stacked -= image_stack[stack_head];
        image_stack[stack_head] = images->current_image.clone() / stack_base;
        stacked += image_stack[stack_head];

        cv::cvtColor(stacked, im, cv::COLOR_BGR2GRAY);

        cv::Sobel(im, sobelx, CV_32F, 1, 0, 1);
        cv::Sobel(im, sobely, CV_32F, 0, 1, 1);
        cv::cartToPolar(sobelx, sobely, magnitudes, angles, true);

        cv::resize(sobely, sobely, cv::Size(128,128), 0,0, cv::INTER_AREA );
        cv::resize(magnitudes, magnitudes, cv::Size(128,128), 0,0, cv::INTER_AREA );

        result = magnitudes + sobely;
        cv::GaussianBlur(result, result, cv::Size(0,0), 9);

        cv::reduce(result, result, 1, cv::REDUCE_SUM);
        cv::normalize(result, result, -1, 1, cv::NORM_MINMAX);

        position = getPosition(result);
        window_offset = -2;
        shift = (position.y+window_offset)*2;
        shift = -((shift-(center_cam))*4);

        //std::cout << shift << std::endl;

        if (!full_stack && stack_head == (stack_base-1))
            full_stack = true;
        stack_head = (stack_head+1)%stack_base;
        // std::cout << "fuzzy tufted new" << std::endl;
    }
    else{
        images->shift_average.base = 7;
        cv::cvtColor(images->current_image, im, cv::COLOR_BGR2GRAY);
        angles = findAngles(im);
        cv::matchTemplate(angles, images->synthetic_template, result1, cv::TM_CCOEFF_NORMED);
        cv::matchTemplate(angles, images->synthetic_template_inverted, result2, cv::TM_CCOEFF_NORMED);
        cv::absdiff(result1, result2, result);
        position = getPosition(result);
        window_offset = 5;
        shift = (position.y+window_offset)*2;
        shift = -((shift-(center_cam))*4);
        //std::cout << "printed/v202" << std::endl;
    }

    //std::cout << shift << std::endl;

    if (true)//confidence(images,result))
        images->shift = shift;
    else
        images->shift = 0;
}

bool loaded = false;
void getMovement(Images *local_set){
    std::chrono::time_point<std::chrono::system_clock> start_comp, end_comp;

    std::chrono::milliseconds dF = std::chrono::duration_cast<std::chrono::milliseconds>(local_set->c_stamp-local_set->p_stamp);
    local_set->frame_gap = dF.count();

        start_comp = std::chrono::system_clock::now();
        ///////////////////////////////////////////////////////////////////////////////////////
        if (loaded && dF.count() <=300){
            computeMovement(local_set);
        }
        local_set->previous_image = local_set->current_image.clone();
        local_set->p_stamp = local_set->c_stamp;
        loaded = true;
        ///////////////////////////////////////////////////////////////////////////////////////
        end_comp = std::chrono::system_clock::now();

    std::chrono::milliseconds dV = std::chrono::duration_cast<std::chrono::milliseconds>(end_comp-start_comp);
    //std::cout << " dF: " << dF.count() << " dC: " << dV.count() << " " << std::endl;
}