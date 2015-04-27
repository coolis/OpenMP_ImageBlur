#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<sstream>
#include<omp.h>

using namespace std;

struct RGB {
    int r;
    int g;
    int b;
    RGB(int R, int G, int B) : r(R), g(G), b(B) {};
};

struct Grey {
    int value;
    Grey(int v) : value(v) {};
};

template <class T>
class Image {
public:
    string width;
    string height;
    string max_value;
    string type;
    vector<vector<T> > pixels;
    Image() {};
    Image(string w, string h, string mv, string t) : width(w), height(h), max_value(mv), type(t) {};
};

class ImageReaderWriter {
public:
    Image<RGB> ppmReader(ifstream &inFile) {
        string width, height, max_value, type;
        inFile >> type >> width >> height >> max_value;
        if (type != "P3") {
            cout << "Wrong type of PPM" << endl;
            exit(0);
        }
        int w = stoi(width);
        int h = stoi(height);
        
        Image<RGB> img(width, height, max_value, type);
        for (int i=0; i<h; i++) {
            vector<RGB> row;
            for (int j=0; j<w; j++) {
                string r,g,b;
                if (inFile >> r >> g >> b) {
                    RGB pixel(stoi(r), stoi(g), stoi(b));
                    row.push_back(pixel);
                } else
                    exit(0);
            }
            img.pixels.push_back(row);
        }
        return img;
    };

    Image<Grey> pgmReader(ifstream &inFile) {
        string width, height, max_value, type;
        inFile >> type >> width >> height >> max_value;
        if (type != "P2") {
            cout << "Wrong type of PGM" << endl;
            exit(0);
        }
        int w = stoi(width);
        int h = stoi(height);
        
        Image<Grey> img(width, height, max_value, type);
        for (int i=0; i<h; i++) {
            vector<Grey> row;
            for (int j=0; j<w; j++) {
                string v;
                if (inFile >> v) {
                    Grey pixel(stoi(v));
                    row.push_back(pixel);
                } else
                    exit(0);
            }
            img.pixels.push_back(row);
        }
        return img;
    };

    string ppmWriter(Image<RGB> &img) {
        string res = "";
        res += img.type+"\n";
        res += img.width+" ";
        res += img.height+"\n";
        res += img.max_value+"\n";
        for (int i=0; i<img.pixels.size(); i++) {
            string row = "";
            for (int j=0; j<img.pixels[i].size(); j++) {
                row += to_string(img.pixels[i][j].r)+" ";
                row += to_string(img.pixels[i][j].g)+" ";
                row += to_string(img.pixels[i][j].b)+" ";
            }
            res += row+"\n";
        }
        return res;
    }

    string pgmWriter(Image<Grey> &img) {
        string res = "";
        res += img.type+"\n";
        res += img.width+" ";
        res += img.height+"\n";
        res += img.max_value+"\n";
        for (int i=0; i<img.pixels.size(); i++) {
            string row = "";
            for (int j=0; j<img.pixels[i].size(); j++) {
                row += to_string(img.pixels[i][j].value)+" ";
            }
            res += row+"\n";
        }
        return res;
    }
};

class ImageProcessor {
private:
    Image<Grey> stencil; 
    vector<vector<float> > kernel;
public:
    ImageProcessor(Image<Grey> s) {
        int maxValue = stoi(s.max_value);
        stencil = s;
        for (int i=0; i<s.pixels.size(); i++) {
            vector<float> row;
            for (int j=0; j<s.pixels[i].size(); j++) {
                //int v = -4+(float)(8*s.pixels[i][j].value)/maxValue; 
                float v = (float)s.pixels[i][j].value/maxValue;
                row.push_back(v);
            }
            kernel.push_back(row);
        }
    };
    void process(Image<RGB> &img, Image<RGB> &outImage) {
        #pragma omp parallel for num_threads(4)
        for (int imgRow = 0; imgRow < img.pixels.size(); imgRow++) {
            for (int imgCol = 0; imgCol < img.pixels[imgRow].size(); imgCol++) {
                float r=0, g=0, b=0;
                for (int kernelRow = 0; kernelRow < kernel.size(); kernelRow++) {
                    for (int kernelCol = 0; kernelCol < kernel[kernelRow].size(); kernelCol++) {
                        int y = imgRow - (kernel.size()/2 - kernelRow);
                        int x = imgCol - (kernel[kernelRow].size()/2  - kernelCol);
                        if ( x >= 0 && x < img.pixels[imgRow].size() && y >= 0 && y < img.pixels.size()) {
                            r += img.pixels[y][x].r * kernel[kernelRow][kernelCol];
                            g += img.pixels[y][x].g * kernel[kernelRow][kernelCol];
                            b += img.pixels[y][x].b * kernel[kernelRow][kernelCol];
                        }
                    }
                }
                RGB newPixel((int)r, (int)g, (int)b);
                outImage.pixels[imgRow][imgCol] = newPixel;
            }
        }
    }
};

int main(int argc, char *argv[]) {

    if (argc < 3) {
        cout << "Please tell the path of image and stencil" << endl;
        exit(0);
    }

    int counter, n;
    if (argc >= 4) {
        counter = atoi(argv[3]); 
    } else
        counter = 1;

    if (argc >= 5) {
        n = atoi(argv[4]);
    } else
        n = 1;

    ifstream inFile, inStencil;
    string file_path = argv[1];
    string stencil_path = argv[2];
    inFile.open(file_path, ifstream::in); 
    if (!inFile.is_open()) {
        cout << "Failed to open image " << file_path << endl; 
        exit(0);
    }
    inStencil.open(stencil_path, ifstream::in);
    if (!inStencil.is_open()) {
        cout << "Failed to open stencil " << stencil_path << endl;
        exit(0);
    }

    ImageReaderWriter img_rw;
    Image<RGB> myImage = img_rw.ppmReader(inFile);
    Image<Grey> stencil = img_rw.pgmReader(inStencil);
    Image<RGB> outImage = myImage;
    ImageProcessor imgP(stencil);
    for (int i=0; i<counter; i++) {
        imgP.process(myImage, outImage);
        myImage = outImage;
    }

    string img_stream = img_rw.ppmWriter(outImage);
    ofstream outFile ("output.ppm", ofstream::out);
    if (outFile.is_open()) {
        outFile << img_stream;
    }
    cout << "Processing finished" << endl;
    inFile.close();
    inStencil.close();
    outFile.close();

    return 0;
}
