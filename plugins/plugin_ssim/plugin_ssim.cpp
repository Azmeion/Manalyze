#include "plugin_framework/plugin_interface.h"
#include "icon_analysis.h"
#include "lodepng.h"
#include <fstream>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <math.h>
#include <iostream>

namespace plugin
{

class HelloWorldPlugin : public IPlugin
{

    int get_api_version() const override { return 1; }

    pString get_id() const override {
        return boost::make_shared<std::string>("icon_analyze");
    }

    pString get_description() const override {
        return boost::make_shared<std::string>("compare the PE icon with a database of icons");
    }

    pResult analyze(const mana::PE& pe) override
    {
        pResult res = create_result();

        std::vector<std::string> info_en_plus;

        mana::shared_resources resources = pe.get_resources();

        std::vector<BYTE> buffer;

        for (mana::pResource r : *resources ) {
            if (( *(r->get_type()) == "RT_GROUP_ICON") || ( *(r->get_type()) == "RT_GROUP_CURSOR")) {

            buffer = mana::reconstruct_icon(r->interpret_as<mana::pgroup_icon_directory>(), *resources);

            }
        }

        // We need an ICONDIR to hold the data
        ICONDIR * pIconDir = new ICONDIR; // TODO : Utiliser des jolis pointeurs.
        ICONDIR * pIconDir2 = new ICONDIR; // TODO : Utiliser des jolis pointeurs.
        std::vector<BYTE> buffer2;
        //fileToBuf("/home/shadows/Documents/Manalyze/Manalyze/plugins/plugin_ssim/dataicon/app.ico", buffer);
        fileToBuf("/home/shadows/Documents/Manalyze/ManaNous/Manalyze/plugins/plugin_ssim/dataicon/app.ico", buffer2);
        std::vector<dataIcon> math, office;
        float result = 0, h_result = 0;
        WORD info_dim = 0, dim_m, dim_o;
        int nbBytesRead;

        if (!buffer.empty())
        {
            nbBytesRead = 0;
            icoHeaderRead(pIconDir, buffer, nbBytesRead);
            math = icoEntriesRead(pIconDir, buffer, nbBytesRead);//, math);
            delete pIconDir;
            buffer.clear();
        }

        std::cout << std::endl;

        if (!buffer2.empty())
        {
            nbBytesRead = 0;
            icoHeaderRead(pIconDir2, buffer2, nbBytesRead);
            office = icoEntriesRead(pIconDir2, buffer2, nbBytesRead);//, office);
            delete pIconDir2;
            buffer2.clear();
        }

        std::cout << std::endl;

        // if(math.front().getDim() && office.front().getDim())
        // {
        //     h_result = office.front().ssim(math.front());
        //     info_dim = math.front().getDim();
        // }

        for (dataIcon im : math) {
            for (dataIcon io : office) {
                dim_m = im.getDim();
                dim_o = io.getDim();
                if (dim_m && dim_o && dim_m == dim_o) {
                    result = im.ssim(io);
                    std::cout << im.getMean() << " ; " << io.getMean() << std::endl;
                    std::cout << im.getVariance() << " ; " << io.getVariance() << std::endl;
                    std::cout << dim_o << " : " << result << std::endl;
                    if (h_result < result) {
                        h_result = result;
                        info_dim = dim_m;
                    }
                }
            }
        }


        res->set_level(SUSPICIOUS);
        res->set_summary("Similarity with an icon");

        std::string test_visu = "Discord : " + std::to_string(h_result);

        info_en_plus.push_back(test_visu);

        res->add_information("Highest similarity ", h_result);
        res->add_information("Icon size ", std::to_string(info_dim) + "x" + std::to_string(info_dim));
        res->add_information("Similar icons ", info_en_plus);
/**/
        return res;
    }
};

// ----------------------------------------------------------------------------

extern "C"
{
    PLUGIN_API IPlugin* create() { return new HelloWorldPlugin(); }
    PLUGIN_API void destroy(IPlugin* p) { delete p; }
};

} //!namespace plugin