#include <fstream>
#include <math.h>
#include <iostream>

#include "icon_analysis.h"
#include "lodepng.h"

namespace plugin
{

DataIcon::DataIcon(const std::string& name, const std::vector<RGBQuad>& rgba_colors, const Word& dimension) : _name(name), _dimension(dimension), _mean(0), _variance(0)
{
    _gray_table.reserve(dimension * dimension);

    float luma,r,g,b;
    for (Word i = 0; i < dimension; ++i)
    {
        for (Word j = 0; j < dimension; ++j)
        {
            r = (float) rgba_colors[i * dimension + j].rgb_red;
            g = (float) rgba_colors[i * dimension + j].rgb_green;
            b = (float) rgba_colors[i * dimension + j].rgb_blue;

            luma = 0.2126f * r + 0.7152f * g + 0.0722f * b;
            _mean += luma;
            _gray_table.push_back((Byte) luma);
        }
    }

    _mean /= dimension*dimension;

    for (Byte gray : _gray_table)
    {
        luma = gray - _mean;
        _variance += luma * luma;
    }

    _variance /= (dimension * dimension - 1);
}

// ----------------------------------------------------------------------------

float DataIcon::ssim(DataIcon& database_icon)
{
    float cov = 0.0, sim;
    float c1 = 0.01f * (_dimension - 1);
    c1 = c1*c1;
    float c2 = 0.03f * (_dimension - 1);
    c2 = c2*c2;

    std::vector<Byte>::iterator di_it = database_icon._gray_table.begin();

    // Covariance calculation
    for (Byte i_it : _gray_table) {
        cov += (i_it - _mean) * (*di_it - database_icon._mean);
        di_it++;
    }

    cov /= _dimension * _dimension - 1;

    // Ssim calculation
    sim = (2 * _mean * database_icon._mean + c1) * (2 * abs(cov) + c2);
    sim /= (_mean * _mean + database_icon._mean * database_icon._mean + c1) * (_variance + database_icon._variance + c2);

    return sim;
}

// ----------------------------------------------------------------------------

void ico_header_read(boost::shared_ptr<IconDir>& p_icondir, const std::vector<Byte>& buffer, Long& nb_bytes_read) {
    
    memcpy(&(p_icondir->id_reserved), &buffer[nb_bytes_read], sizeof(Word));
    nb_bytes_read += sizeof(Word);

    memcpy(&(p_icondir->id_type), &buffer[nb_bytes_read], sizeof(Word));
    nb_bytes_read += sizeof(Word);

    memcpy(&(p_icondir->id_count), &buffer[nb_bytes_read], sizeof(Word));
    nb_bytes_read += sizeof(Word);

    for (Long i = 0; i < p_icondir->id_count ; ++i)
    {
        p_icondir->id_entries.push_back(*((IconDirentry*)(&buffer[nb_bytes_read])));
        nb_bytes_read += sizeof(IconDirentry);
    }
}

// ----------------------------------------------------------------------------

template<typename T> 
void vector_cpy(std::vector<T>& vector, const boost::int64_t& nbr_pixel, const std::vector<Byte>& buffer, Long& nb_bytes_read){
    vector.reserve(nbr_pixel);
    vector.assign((T*)(&buffer[nb_bytes_read]), (T*)(&buffer[nb_bytes_read + nbr_pixel * sizeof(T)]));
    nb_bytes_read += nbr_pixel * sizeof(T);
};

// ----------------------------------------------------------------------------

Icon::Icon(IconDirentry& icondirentry, const std::vector<Byte>& buffer, const std::string& name, Long& nb_bytes_read) : _png(false), _nbr_pixel(-1)
{
    unsigned b_height = icondirentry.b_height;
    unsigned b_width = icondirentry.b_width;
    Word dimension = b_height;
    const Byte Png_Sig[8] = {137,80,78,71,13,10,26,10};

    // Regular icon sizes only
    if (!(b_height % MIN_ICON_SIZE) && !(b_width % MIN_ICON_SIZE))
    {
        _p_icondir = boost::shared_ptr<IconDirentry>(new TagIconDirentry(icondirentry));

        // Value 0 of b_height and b_width equals 256
        if (!(_nbr_pixel = b_height * b_width)) {
            _nbr_pixel = 65536;
            dimension = 256;
        }

        nb_bytes_read = icondirentry.dw_image_offset;

        // Icon can be a Bitmap or a PNG
        if (!strncmp((const char*)&buffer[nb_bytes_read], (char*)Png_Sig, 8))
        {
            _png = true;
            std::vector<Byte> image;
            unsigned error = lodepng::decode(image, b_height, b_width, (const unsigned char*)(&buffer[nb_bytes_read]), (size_t)icondirentry.dw_bytes_in_res);
            
            if(error) {
                std::cout << "PNG decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
            }
            
            else
            {
                std::vector<RGBQuad> ic_color;
                boost::shared_ptr<RGBQuad> rgb(new RGBQuad);
                for (std::vector<Byte>::iterator p = image.begin() ; p!=image.end(); p++)
                {
                    rgb->rgb_red = (*p);
                    p++;
                    rgb->rgb_green = (*p);
                    p++;
                    rgb->rgb_blue = (*p);
                    p++;
                    rgb->rgb_reserved = (*p);
                    ic_color.push_back(*rgb);
                }
                _data = DataIcon(name, ic_color, dimension);
            }
        }
        else 
        {
            boost::shared_ptr<IconImage> p_icon_image(new IconImage);
            memcpy(&(p_icon_image->ic_header), &buffer[nb_bytes_read], sizeof(BitmapInfoHeader));
            nb_bytes_read += sizeof(BitmapInfoHeader);

            
            vector_cpy(p_icon_image->ic_colors, _nbr_pixel, buffer, nb_bytes_read);
            //vector_cpy(p_icon_image->ic_xor, _nbr_pixel, buffer, nb_bytes_read);      // Experimental
            //vector_cpy(p_icon_image->ic_and, _nbr_pixel, buffer, nb_bytes_read);      // Experimental
            _data = DataIcon(name, p_icon_image->ic_colors, dimension);
        }
    }
}

// ----------------------------------------------------------------------------

std::vector<DataIcon> ico_entries_read(const boost::shared_ptr<IconDir>& p_icondir, const std::vector<Byte>& buffer, const std::string& name, Long& nb_bytes_read) {
    std::vector<DataIcon> data;
    for (IconDirentry ico : p_icondir->id_entries)
    {
        boost::shared_ptr<Icon> i(new Icon(ico, buffer, name, nb_bytes_read));

        // If the icon size is not a regular one, don't take it
        if (i->get_nbr_pixel() != -1)
        {
            data.push_back(i->get_data());
        }
    }
    return data;
}

// ----------------------------------------------------------------------------

void file_to_buf(const std::string& fname, std::vector<Byte>& buffer)
{
    std::ifstream is(fname, std::ios::binary);

    if(is)
    {
        is.seekg (0, is.end);
        int length = is.tellg();
        is.seekg (0, is.beg);

        buffer.reserve(length);
        buffer.assign(std::istreambuf_iterator<char>(is),
                        std::istreambuf_iterator<char>());
    }
}

// ----------------------------------------------------------------------------

bool ico_sort(const DataIcon& ico1, const DataIcon& ico2) {
    return ico1.get_dimension() < ico2.get_dimension();
}

} /* !namespace plugin */