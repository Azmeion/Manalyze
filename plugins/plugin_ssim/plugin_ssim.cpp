#include "plugin_framework/plugin_interface.h"
#include "json.hpp" // Licence MIT, presente dans le fichier, donc on est bon.
#include "icon_analysis.h"
#include "lodepng.h"
#include <fstream>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <math.h>
#include <iostream>
#include <boost/filesystem.hpp>

using json = nlohmann::json;

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

        mana::shared_resources resources = pe.get_resources();

        std::vector<BYTE> buffer, buffer2;

        for (mana::pResource r : *resources ) {
            if (( *(r->get_type()) == "RT_GROUP_ICON") || ( *(r->get_type()) == "RT_GROUP_CURSOR")) {

            buffer = mana::reconstruct_icon(r->interpret_as<mana::pgroup_icon_directory>(), *resources);

            }
        }

        if (!buffer.empty())
        {
            // We need an ICONDIR to hold the data
            ICONDIR * pIconDir = new ICONDIR; // TODO : Utiliser des jolis pointeurs.

            std::vector<dataIcon> database, pe_dataIcon, db_dataIcon;
            float result = 0, h_result = 0;
            WORD info_dim = 0, dim_m, dim_o, dim_i;
            LONG i = 0, nbBytesRead = 0, l;
            std::string p_ico_gen = "./../plugins/plugin_ssim/dataicon/ico/", p_json_gen = "./../plugins/plugin_ssim/dataicon/json/", p_ico, p_json, fname, info_name;
            json jo; 

            icoHeaderRead(pIconDir, buffer, nbBytesRead);
            pe_dataIcon = icoEntriesRead(pIconDir, buffer, "pe", nbBytesRead);
            std::sort(pe_dataIcon.begin(), pe_dataIcon.end(), icoSort);

            delete pIconDir;
            buffer.clear();

            // Creation of the database
            for (boost::filesystem::directory_entry & f : boost::filesystem::directory_iterator(p_ico_gen))
            {
                fname = f.path().leaf().string();
                fname = fname.substr(0, fname.size() - 4);

                p_json = p_json_gen + fname + ".json";
                
                if (boost::filesystem::exists(p_json)) // The Json file is already in the database
                {
                    std::ifstream iput(p_json);
                    iput >> jo; 
                    iput.close();

                    for (auto & el : jo.items())
                    {
                        if (el.value() != nullptr)
                        {
                            auto name = el.value().at("name").get<std::string>();
                            auto grayTable = el.value().at("grayTable").get<std::vector<BYTE>>();
                            auto dim = (WORD) std::stoi(el.key()) * MIN_ICON_SIZE;
                            auto mean = el.value().at("mean").get<float>();
                            auto variance = el.value().at("variance").get<float>();

                            dataIcon io(name, grayTable, dim, mean, variance);

                            database.push_back(io);
                        }
                    }   
                }
                else                                // the Json file is not in the database, so we create it
                {
                    p_ico = f.path().string();

                    fileToBuf(p_ico, buffer2);

                    if (!buffer2.empty())
                    {
                        nbBytesRead = 0;
                        pIconDir = new ICONDIR;
                        icoHeaderRead(pIconDir, buffer2, nbBytesRead);
                        db_dataIcon = icoEntriesRead(pIconDir, buffer2, fname, nbBytesRead);

                        delete pIconDir;
                        buffer2.clear();
                        
                        for (dataIcon io : db_dataIcon)
                        {
                            database.push_back(io);
                            dim_o = io.getDim() / MIN_ICON_SIZE;
                            jo[dim_o]["name"] = fname;
                            jo[dim_o]["grayTable"] = *(io.getGrayTable());
                            jo[dim_o]["mean"] = io.getMean();
                            jo[dim_o]["variance"] = io.getVariance();
                        }

                        std::ofstream oput(p_json);
                        oput << jo << std::endl;
                        oput.close();
                    }
                }
            }

            std::sort(database.begin(), database.end(), icoSort);
            l = database.size();

            // Comparison
            for (dataIcon im : pe_dataIcon)
            {
                dim_i = database[i].getDim();
                dim_m = im.getDim();
                while (i < l && dim_i <= dim_m)
                {
                    if (dim_i == dim_m)
                    {
                        result = im.ssim(database[i]);
                        if (h_result <= result)
                        {
                            h_result = result;
                            info_name = database[i].getName();
                            info_dim = dim_m;
                        }
                }
                    ++i;
                    dim_i = database[i].getDim();
                }
            }

            // Result
            res->set_level(SUSPICIOUS);

            if (h_result == 1)
            {
                res->set_summary("Match");
            }
            else if (h_result > 0.95)
            {
                res->set_summary("Highly similar");
            }
            else if (h_result > 0.75)
            {
                res->set_summary("Quite similar");
            }
            else if (h_result > 0.4)
            {
                res->set_summary("Little similar");
            }
            else
            {
                res->set_summary("Not similar");
            }

            res->add_information("Name ", info_name);
            res->add_information("Icon size ", std::to_string(info_dim) + "x" + std::to_string(info_dim));
            res->add_information("Similarity ", h_result);
/**/
        }
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