#ifndef TRUC
#define TRUC

#include <bitset>
#include <vector>
#include <sstream>
#include <boost/cstdint.hpp>

#define MIN_ICON_SIZE 16 // For testing purposes.

typedef boost::uint8_t BYTE;
typedef boost::uint16_t WORD;
typedef boost::uint32_t DWORD;
typedef boost::uint64_t LONG;

typedef class tagICONDIRENTRY
{
    BYTE        bWidth;          // Width, in pixels, of the image
    BYTE        bHeight;         // Height, in pixels, of the image
    BYTE        bColorCount;     // Number of colors in image (0 if >=8bpp)
    BYTE        bReserved;       // Reserved (must be 0)
    WORD        wPlanes;         // Color Planes
    WORD        wBitCount;       // Bits per pixel
    DWORD       dwBytesInRes;    // How many bytes in this resource?
    DWORD       dwImageOffset;   // Where in the file is this image?
    public :
    BYTE getbWidth() const {return bWidth;};
    BYTE getbHeight() const {return bHeight;};
    DWORD getBytesInRes() const {return dwBytesInRes;};
    DWORD getOffset() const {return dwImageOffset;};

    tagICONDIRENTRY() {};
    ~tagICONDIRENTRY() {};
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct tagICONDIR
{
    WORD           idReserved;   // Reserved (must be 0)
    WORD           idType;       // Resource Type (1 for icons)
    WORD           idCount;      // How many images?
    std::vector<ICONDIRENTRY>   idEntries; // An entry for each image (idCount of 'em)
} ICONDIR, *LPICONDIR;

typedef struct tagRGBQUAD {
  BYTE rgbBlue;
  BYTE rgbGreen;
  BYTE rgbRed;
  BYTE rgbReserved; // Alpha for 32-bit images.
} RGBQUAD;

typedef struct tagBITMAPINFOHEADER {
  DWORD biSize;
  LONG  biWidth;
  LONG  biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG  biXPelsPerMeter;
  LONG  biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagICONIMAGE
{
  BITMAPINFOHEADER     icHeader;   // DIB header
  std::vector<RGBQUAD> icColors;   // Color table
  std::vector<BYTE>    icXOR;      // DIB bits for XOR mask
  std::vector<BYTE>    icAND;      // DIB bits for AND mask
} ICONIMAGE, *LPICONIMAGE;

struct IHDR {
  DWORD width;
  DWORD height;
  BYTE  depth;
  BYTE  colorType;
  BYTE  compression;
  BYTE  filter;
  BYTE  interlace;
};

struct PNGIMAGE
{
  IHDR                  icHeader;   // PNG Header
  std::vector<RGBQUAD>  icColors;   // Color table
  std::vector<BYTE>     icXOR;      // DIB bits for XOR mask
  std::vector<BYTE>     icAND;      // DIB bits for AND mask
};

class dataIcon {
  std::string name;
  std::vector<BYTE> grayTable;
  WORD dim;
  float mean;
  float variance;

  public :
    dataIcon() : dim(0), mean(0), variance(0) {};
    dataIcon(const std::string &, const std::vector<RGBQUAD> &, const WORD &);   // Create a dataIcon to compare it with the database
                                      // TODO : Add height and width of the used bitmap image for different sizes
    dataIcon(const std::string & n, const std::vector<BYTE> & g, const WORD & d, const float & m, const float & v) : name(n), grayTable(g), dim(d), mean(m), variance(v) {};
    ~dataIcon() {};

    std::string getName() const { return name; };
    std::vector<BYTE>* getGrayTable() { return &grayTable; }; // Par pointeur pour eviter la copie retournee. (Passage en const ?)
    WORD getDim() const { return dim; };
    float getMean() const { return mean; };
    float getVariance() const { return variance; };
    float ssim (dataIcon &);            // Value between 0 and 1
};

//Single icon.
class Icon {
  ICONDIRENTRY * pIconDir = nullptr;
  dataIcon data;
  bool png = false;
  unsigned long nbrPixel = 0;

  public : 
    Icon() {};
    Icon(ICONDIRENTRY & pIconDir, const std::vector<BYTE> & buffer, const std::string &, LONG & nbBytesRead);
    ~Icon() {};
    boost::int64_t getNbrPixel() const {return nbrPixel;};
    dataIcon getData() const {return data;};
};

void fileToBuf(const std::string &, std::vector<BYTE> &);
std::vector<dataIcon> icoEntriesRead(const ICONDIR *, const std::vector<BYTE> &, const std::string &, LONG &);
void icoHeaderRead(ICONDIR *, const std::vector<BYTE> &, LONG &);
bool icoSort(const dataIcon &, const dataIcon &);

#endif