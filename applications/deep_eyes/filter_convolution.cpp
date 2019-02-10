//Just the example from caffe with just few hack to dig in the data structures


#ifdef USE_CAFFE
#include <caffe/caffe.hpp>

#ifdef USE_OPENCV
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#endif  // USE_OPENCV
#include <algorithm>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <QImage>
#include"hdi/utils/visual_utils.h"

#ifdef USE_OPENCV
using namespace caffe;  // NOLINT(build/namespaces)
using std::string;

/* Pair (label, confidence) representing a prediction. */
typedef std::pair<string, float> Prediction;

class Classifier {
    public:
        Classifier(const string& model_file,
                 const string& trained_file,
                 const string& mean_file,
                 const string& label_file);

        std::vector<Prediction> Classify(const cv::Mat& img, int N = 5);

        void function42(const string& output_folder)const;

    private:
        void SetMean(const string& mean_file);

        std::vector<float> Predict(const cv::Mat& img);

        void WrapInputLayer(std::vector<cv::Mat>* input_channels);

        void Preprocess(const cv::Mat& img,
                      std::vector<cv::Mat>* input_channels);

    private:
        shared_ptr<Net<float> > net_;
        cv::Size input_geometry_;
        int num_channels_;
        cv::Mat mean_;
        std::vector<string> labels_;
};

Classifier::Classifier(const string& model_file,
                       const string& trained_file,
                       const string& mean_file,
                       const string& label_file) {
#ifdef CPU_ONLY
  Caffe::set_mode(Caffe::CPU);
#else
  Caffe::set_mode(Caffe::GPU);
#endif

    /* Load the network. */
    net_.reset(new Net<float>(model_file, TEST));
    net_->CopyTrainedLayersFrom(trained_file);

  CHECK_EQ(net_->num_inputs(), 1) << "Network should have exactly one input.";
  CHECK_EQ(net_->num_outputs(), 1) << "Network should have exactly one output.";

  Blob<float>* input_layer = net_->input_blobs()[0];
  num_channels_ = input_layer->channels();
  CHECK(num_channels_ == 3 || num_channels_ == 1) << "Input layer should have 1 or 3 channels.";
  input_geometry_ = cv::Size(input_layer->width(), input_layer->height());

  /* Load the binaryproto mean file. */
  SetMean(mean_file);

  /* Load labels. */
  std::ifstream labels(label_file.c_str());
  CHECK(labels) << "Unable to open labels file " << label_file;
  string line;
  while (std::getline(labels, line))
    labels_.push_back(string(line));

  Blob<float>* output_layer = net_->output_blobs()[0];
  CHECK_EQ(labels_.size(), output_layer->channels()) << "Number of labels is different from the output layer dimension.";
}


void Classifier::function42(const string& output_folder)const{
    auto& layer_names = net_->layer_names();
    for(auto name: layer_names){
      std::cout << name << std::endl;
    }
    auto& layers = net_->layers();
    int layer_id = 0;
    for(auto& layer: layers){
        if(std::string(layer->type()).compare("Convolution")==0){
            std::cout << "--------------------" << std::endl;
            std::cout << "type:\t"<< layer->type() << std::endl;

            caffe::LayerParameter layer_param = layer->layer_param();
            if(layer_param.has_convolution_param()){
                std::cout << "group:\t"<< layer_param.convolution_param().group() << std::endl;
            }

            //learnable parameters
            std::vector<caffe::shared_ptr<Blob<float> >>& learn_params = layer->blobs();
            std::cout << "|learnable params|:\t"<< learn_params.size() << std::endl;
            for(int i = 0; i < learn_params.size(); ++i){
                auto& shape = learn_params[i]->shape();
                std::cout << "\t (" << shape.size() << ") ";
                for(auto l: shape){
                  std::cout << l << "  ";
                }
                std::cout << std::endl;
            }

            int num_filters = layer->blobs()[0]->shape()[0];
            int depth = layer->blobs()[0]->shape()[1];
            int width = layer->blobs()[0]->shape()[2];
            int height = layer->blobs()[0]->shape()[3];
            //analysis
            std::vector<std::map<unsigned int,double>> D(num_filters);

            if(depth == 3){
                for(int f = 0; f < num_filters; ++f){
                    QImage img(width,height,QImage::Format_ARGB32);
                    for(int j = 0; j < height; ++j){
                        for(int i = 0; i < width; ++i){
                          //  std::cout << layer->blobs()[0]->data_at(f,0,j,i) << " ";
                            double multF = 300;
                            auto pixel = qRgb(125+layer->blobs()[0]->data_at(f,0,j,i)*multF, 125+layer->blobs()[0]->data_at(f,1,j,i)*multF, 125+layer->blobs()[0]->data_at(f,2,j,i)*multF);
                            img.setPixel(i,j,pixel);
                        }
                        //std::cout << std::endl;
                    }
                    img = img.scaledToWidth(100);
                    auto file_name = QString("%1/filter_%2.png").arg(output_folder.c_str()).arg(f);
                    //std::cout << file_name.toStdString() << std::endl;
                    img.save(file_name);
                }
            }

           //if(depth == 3)
           {
                caffe::LayerParameter layer_param = layer->layer_param();
                int group = layer_param.convolution_param().group();

                //inefficient...
                #pragma omp parallel for
                for(int w = 0; w < num_filters; ++w){
                    for(int f = 0; f < num_filters; ++f){
                        D[w][f] = 0;
                        if(w == f){continue;}
                        if((w/(num_filters/group)) != (f/(num_filters/group))){continue;}
                        for(int j = 0; j < height; ++j){
                            for(int i = 0; i < width; ++i){
                                for(int d = 0; d < depth; ++d){
                                    double v = layer->blobs()[0]->data_at(f,d,j,i) * layer->blobs()[0]->data_at(w,d,j,i);
                                    D[w][f] += v;
                                }
                            }
                        }
                        if(D[w][f] < 0){ //RELU
                            D[w][f] = 0;
                        }
                    }
                }

                auto file_name = QString("%1/D_%2.png").arg(output_folder.c_str()).arg(layer_id);
                //auto D_img = hdi::utils::imageFromZeroCenteredSparseMatrix(D);
                QImage D_img = hdi::utils::imageFromSparseMatrix(D);
                D_img = D_img.scaledToWidth(D_img.width()*3);
                D_img.save(file_name);
            }

            ++layer_id;
        }
    }
}


static bool PairCompare(const std::pair<float, int>& lhs,
                        const std::pair<float, int>& rhs) {
  return lhs.first > rhs.first;
}

/* Return the indices of the top N values of vector v. */
static std::vector<int> Argmax(const std::vector<float>& v, int N) {
  std::vector<std::pair<float, int> > pairs;
  for (size_t i = 0; i < v.size(); ++i)
    pairs.push_back(std::make_pair(v[i], i));
  std::partial_sort(pairs.begin(), pairs.begin() + N, pairs.end(), PairCompare);

  std::vector<int> result;
  for (int i = 0; i < N; ++i)
    result.push_back(pairs[i].second);
  return result;
}

/* Return the top N predictions. */
std::vector<Prediction> Classifier::Classify(const cv::Mat& img, int N) {
  std::vector<float> output = Predict(img);

  N = std::min<int>(labels_.size(), N);
  std::vector<int> maxN = Argmax(output, N);
  std::vector<Prediction> predictions;
  for (int i = 0; i < N; ++i) {
    int idx = maxN[i];
    predictions.push_back(std::make_pair(labels_[idx], output[idx]));
  }

  return predictions;
}

/* Load the mean file in binaryproto format. */
void Classifier::SetMean(const string& mean_file) {
  BlobProto blob_proto;
  ReadProtoFromBinaryFileOrDie(mean_file.c_str(), &blob_proto);

  /* Convert from BlobProto to Blob<float> */
  Blob<float> mean_blob;
  mean_blob.FromProto(blob_proto);
  CHECK_EQ(mean_blob.channels(), num_channels_)
    << "Number of channels of mean file doesn't match input layer.";

  /* The format of the mean file is planar 32-bit float BGR or grayscale. */
  std::vector<cv::Mat> channels;
  float* data = mean_blob.mutable_cpu_data();
  for (int i = 0; i < num_channels_; ++i) {
    /* Extract an individual channel. */
    cv::Mat channel(mean_blob.height(), mean_blob.width(), CV_32FC1, data);
    channels.push_back(channel);
    data += mean_blob.height() * mean_blob.width();
  }

  /* Merge the separate channels into a single image. */
  cv::Mat mean;
  cv::merge(channels, mean);

  /* Compute the global mean pixel value and create a mean image
   * filled with this value. */
  cv::Scalar channel_mean = cv::mean(mean);
  mean_ = cv::Mat(input_geometry_, mean.type(), channel_mean);
}

std::vector<float> Classifier::Predict(const cv::Mat& img) {
  Blob<float>* input_layer = net_->input_blobs()[0];
  input_layer->Reshape(1, num_channels_,
                       input_geometry_.height, input_geometry_.width);
  /* Forward dimension change to all layers. */
  net_->Reshape();

  std::vector<cv::Mat> input_channels;
  WrapInputLayer(&input_channels);

  Preprocess(img, &input_channels);

  net_->ForwardPrefilled();

  /* Copy the output layer to a std::vector */
  Blob<float>* output_layer = net_->output_blobs()[0];
  const float* begin = output_layer->cpu_data();
  const float* end = begin + output_layer->channels();
  return std::vector<float>(begin, end);
}

/* Wrap the input layer of the network in separate cv::Mat objects
 * (one per channel). This way we save one memcpy operation and we
 * don't need to rely on cudaMemcpy2D. The last preprocessing
 * operation will write the separate channels directly to the input
 * layer. */
void Classifier::WrapInputLayer(std::vector<cv::Mat>* input_channels) {
  Blob<float>* input_layer = net_->input_blobs()[0];

  int width = input_layer->width();
  int height = input_layer->height();
  float* input_data = input_layer->mutable_cpu_data();
  for (int i = 0; i < input_layer->channels(); ++i) {
    cv::Mat channel(height, width, CV_32FC1, input_data);
    input_channels->push_back(channel);
    input_data += width * height;
  }
}

void Classifier::Preprocess(const cv::Mat& img, std::vector<cv::Mat>* input_channels) {
    /* Convert the input image to the input image format of the network. */
    cv::Mat sample;
    if (img.channels() == 3 && num_channels_ == 1)
        cv::cvtColor(img, sample, CV_BGR2GRAY);
    else if (img.channels() == 4 && num_channels_ == 1)
        cv::cvtColor(img, sample, CV_BGRA2GRAY);
    else if (img.channels() == 4 && num_channels_ == 3)
        cv::cvtColor(img, sample, CV_BGRA2BGR);
    else if (img.channels() == 1 && num_channels_ == 3)
        cv::cvtColor(img, sample, CV_GRAY2BGR);
    else
    sample = img;

    cv::Mat sample_resized;
    if (sample.size() != input_geometry_)
        cv::resize(sample, sample_resized, input_geometry_);
    else
        sample_resized = sample;

    cv::Mat sample_float;
    if (num_channels_ == 3)
        sample_resized.convertTo(sample_float, CV_32FC3);
    else
        sample_resized.convertTo(sample_float, CV_32FC1);

    cv::Mat sample_normalized;
    cv::subtract(sample_float, mean_, sample_normalized);

    /* This operation will write the separate BGR planes directly to the
    * input layer of the network because it is wrapped by the cv::Mat
    * objects in input_channels. */
    cv::split(sample_normalized, *input_channels);

    CHECK(reinterpret_cast<float*>(input_channels->at(0).data) == net_->input_blobs()[0]->cpu_data())  << "Input channels are not wrapping the input layer of the network.";
}

int main(int argc, char** argv) {
    if (argc != 7) {
        std::cerr << "Usage: " << argv[0]
                  << " deploy.prototxt network.caffemodel"
                  << " mean.binaryproto labels.txt img.jpg outfolder" << std::endl;
        return 1;
    }

    //::google::InitGoogleLogging(argv[0]);

    string model_file   = argv[1];
    string trained_file = argv[2];
    string mean_file    = argv[3];
    string label_file   = argv[4];
    Classifier classifier(model_file, trained_file, mean_file, label_file);
    classifier.function42(argv[6]);

    string file = argv[5];

    std::cout << "---------- Prediction for "
            << file << " ----------" << std::endl;

    cv::Mat img = cv::imread(file, -1);
    //CHECK(!img.empty()) << "Unable to decode image " << file;
    std::vector<Prediction> predictions = classifier.Classify(img);

    /* Print the top N predictions. */
    for (size_t i = 0; i < predictions.size(); ++i) {
        Prediction p = predictions[i];
        std::cout << std::fixed << std::setprecision(4) << p.second << " - \""
                  << p.first << "\"" << std::endl;
    }
}
#else
#include <iostream>
int main(int argc, char** argv) {
  std::cout << "This example requires OpenCV; compile with USE_OPENCV.";
}
#endif  // USE_OPENCV

#else
#include <iostream>
int main(int argc, char** argv) {
  std::cout << "This example requires Caffe; compile with USE_CAFFE.";
  return 1;
}
#endif  // USE_OPENCV
