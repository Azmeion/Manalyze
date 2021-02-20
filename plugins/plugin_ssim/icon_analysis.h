#pragma once

#include <vector>
#include <sstream>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

namespace plugin
{

#define MIN_ICON_SIZE 16 // For testing purposes.

typedef boost::uint8_t Byte;
typedef boost::uint16_t Word;
typedef boost::uint32_t DWord;
typedef boost::uint64_t Long;

/**
 *  @brief  Structure of a IconDirentry
 * 
 *  b_width           Width, in pixels, of the image
 *  b_height          Height, in pixels, of the image
 *  b_color_count     Number of colors in image (0 if >=8bpp)
 *  b_reserved        Reserved (must be 0)
 *  w_planes          Color Planes
 *  w_bit_count       Bits per pixel
 *  dw_bytes_in_res   Number of bytes in this resource
 *  dw_image_offset   Offset of the image
 */

typedef struct TagIconDirentry
{
    Byte        b_width;
    Byte        b_height;
    Byte        b_color_count;
    Byte        b_reserved;
    Word        w_planes;
    Word        w_bit_count;
    DWord       dw_bytes_in_res;
    DWord       dw_image_offset;
} IconDirentry;

/**
 *  @brief Structure of a IconDir
 * 
 *  id_reserved       Reserved                (must be 0)
 *  id_type           Resource Type           (1 for icons)
 *  id_count          Number of images
 *  id_entries        An entry for each image
 */

typedef struct TagIconDir
{
    Word           id_reserved;
    Word           id_type;
    Word           id_count;
    std::vector<IconDirentry>   id_entries;
} IconDir;

/**
 *  @brief Structure of one pixel of an image
 * 
 *  rgb_blue          Blue color  (Must be between 0 and 255)
 *  rgb_green         Green color (Must be between 0 and 255)
 *  rgb_red           Red color   (Must be between 0 and 255)
 */

typedef struct TagRGBQuad
{
  Byte rgb_blue;
  Byte rgb_green;
  Byte rgb_red;
  Byte rgb_reserved; // Alpha for 32-bit images.
} RGBQuad;

typedef struct TagBitmapInfoHeader
{
  DWord bi_size;
  Long  bi_width;
  Long  bi_height;
  Word  bi_planes;
  Word  bi_bit_count;
  DWord bi_compression;
  DWord bi_size_image;
  Long  bi_x_pels_per_meter;
  Long  bi_y_pels_per_meter;
  DWord bi_clr_used;
  DWord bi_clr_important;
} BitmapInfoHeader;

typedef struct TagIconImage
{
  BitmapInfoHeader     ic_header;   // DIB header
  std::vector<RGBQuad> ic_colors;   // Color table
  std::vector<Byte>    ic_xor;      // DIB bits for XOR mask
  std::vector<Byte>    ic_and;      // DIB bits for AND mask
} IconImage, *LPIconImage;

typedef struct TagPngHeader
{
  DWord width;
  DWord height;
  Byte  depth;
  Byte  color_type;
  Byte  compression;
  Byte  filter;
  Byte  interlace;
} PngHeader;

typedef struct PngImage
{
  PngHeader             ic_header;   // PNG Header
  std::vector<RGBQuad>  ic_colors;   // Color table
  std::vector<Byte>     ic_xor;      // DIB bits for XOR mask
  std::vector<Byte>     ic_and;      // DIB bits for AND mask
} PngImage;


class DataIcon
{
  std::string _name;
  std::vector<Byte> _gray_table;
  Word _dimension;
  float _mean;
  float _variance;

  public :
    DataIcon() : _dimension(0), _mean(0), _variance(0) {};
    DataIcon(const std::string&, const std::vector<RGBQuad>&, const Word&);   // Create a DataIcon to compare it with the database
    DataIcon(const std::string& n, const std::vector<Byte>& g, const Word& d, const float& m, const float& v) : _name(n), _gray_table(g), _dimension(d), _mean(m), _variance(v) {};
    ~DataIcon() {};

    std::string get_name() const { return _name; };
    std::vector<Byte>* get_gray_table() { return &_gray_table; }; // Par pointeur pour eviter la copie retournee. (Passage en const ?)
    Word get_dimension() const { return _dimension; };
    float get_mean() const { return _mean; };
    float get_variance() const { return _variance; };

    /**
     *  @brief  Compare two pictures using the ssim formula
     * 
     *  @param  DataIcon& dataicon  The second compared picture
     * 
     *  @return The result of the ssim comparison
     * 
     *  Value should be between 0 and 1
     */

    float ssim (DataIcon&);
};


class Icon
{
  boost::shared_ptr<IconDirentry> _p_icondir;
  DataIcon _data;
  bool _png = false;
  boost::int64_t _nbr_pixel = 0;

  public : 
    Icon() {};
    Icon(IconDirentry&, const std::vector<Byte>&, const std::string&, Long&);
    ~Icon() {};
    boost::int64_t get_nbr_pixel() const { return _nbr_pixel; };
    DataIcon get_data() const { return _data; };
};


/**
 *  @brief  Put the ico in a buffer
 * 
 *  @param  const std::string& fname    The relative path of the ico
 *  @param  std::vector<Byte>& buffer   The buffer which store the ico's data
 */

void file_to_buf(const std::string&, std::vector<Byte>&);

/**
 *  @brief  Read all the icos in the icondir and put it in a vector
 * 
 *  @param  const boost::shared_ptr<IconDir>& p_icondir   The pointer of the read IconDir
 *  @param  const std::vector<Byte>& buffer               The buffer containing all the ico's data
 *  @param  const std::string& name                       The name of the ico
 *  @param  Long& nb_butes_read                           The number of bytes already read in the buffer
 * 
 *  @return A vector containing all the needed information of every picture in the icondir
 */

std::vector<DataIcon> ico_entries_read(const boost::shared_ptr<IconDir>&, const std::vector<Byte>&, const std::string&, Long&);

/**
 *  @brief  Read the ico's header and put it in a icondir pointer
 * 
 *  @param  const boost::shared_ptr<IconDir>& p_icondir   The pointer of the IconDir
 *  @param  const std::vector<Byte>& buffer               The buffer containing all the ico's data
 *  @param  Long& nb_butes_read                           The number of bytes already read in the buffer
 * 
 *  @return A vector containing all the needed information of every picture in the icondir
 */

void ico_header_read(boost::shared_ptr<IconDir>&, const std::vector<Byte>&, Long&);

/**
 *  @brief  Compare the dimension of two DataIcons
 * 
 *  @param  const DataIcon& ico1  The first DataIcon
 *  @param  const DataIcon& ico2  The second DataIcon
 * 
 *  @return A boolean indicating if the dimension of the first DataIcon is inferior to the other one
 */

bool ico_sort(const DataIcon&, const DataIcon&);

} /* !namespace plugin */