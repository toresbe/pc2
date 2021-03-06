#include <boost/format.hpp>
#include <iostream>
#include <string>
#include "masterlink/telegram.hpp"

std::string payload_header(std::string input) {
    std::string format_string = "\x1b[48;5;99;97m%|=79s|\x1b[0m\n";
    return boost::str(boost::format(format_string) % input);
};

std::string payload_warning(std::string input) {
    input = "\u26A0 " + input;
    std::string format_string = "\x1b[48;5;99;93m%|=81s|\x1b[0m\n";
    return boost::str(boost::format(format_string) % input);
};

std::string payload_format(unsigned int start_byte, unsigned int n_bytes, std::string explanation) {
    if(n_bytes > 1) {
        std::string format_string = "\x1b[48;5;99m\x1b[97m  [%|02X|:%|02X|] \x1b[0m%|-74s|\n";
        return boost::str(boost::format(format_string) % start_byte % (start_byte + n_bytes) % explanation);
    } else {
        std::string format_string = "\x1b[48;5;99m\x1b[97m     [%|02X|] \x1b[0m %|-74s|\n";
        return boost::str(boost::format(format_string) % start_byte % explanation);
    }
};

std::ostream& generic_debug_repr(std::ostream& outputStream, const MasterlinkTelegram *tgram) {
    std::string analysis;
    analysis += payload_header("Unknown telegram, payload");
    int i = 0;
    for(auto x: tgram->payload) {
        analysis += payload_format(i++, 1, boost::str(boost::format("%|02X|") % \
                    (unsigned int) x));
    }
    outputStream << analysis;
    return outputStream;
};

std::ostream& DecodedTelegram::UnknownTelegram::debug_repr(std::ostream& outputStream) {
    return generic_debug_repr(outputStream, this);
};

enum field_type {
    hexbyte,
    ml_source,
    beo4_keycode,
    fixed_width_ascii,
};

typedef struct debug_field {
    unsigned int offset;
    unsigned int length;
    enum field_type type;
    std::string comment;
} debug_field;

typedef std::vector<debug_field> debug_field_list;

std::string format_field(const DecodedTelegram::DecodedTelegram &tgram, debug_field field) {
    if(field.type == hexbyte) {
        assert(field.length == 1);
        return payload_format(field.offset, 1, boost::str(boost::format("%|02X|\t\t\t%||") % \
                    (unsigned int) tgram.payload[field.offset] % field.comment));
    } else if(field.type == fixed_width_ascii) {
        std::string ascii_field = std::string( \
                tgram.payload.begin() + field.offset, \
                tgram.payload.begin() + field.offset + field.length);
        return payload_format(field.offset, field.length, boost::str(boost::format("[%||]\t%||") % \
                    ascii_field % \
                    field.comment));
    } else if(field.type == ml_source) {
        assert(field.length == 1);
        uint8_t source_id = tgram.payload[field.offset];
        std::string source_name;
        Masterlink ml;

        if(ml.source_name.count((Masterlink::source)source_id)) {
            source_name = ml.source_name[(Masterlink::source)source_id];
        } else {
            source_name = "unknown source";
        }

        std::string fmt_string = "[%|02X|] (%|s|)\t\t%||";
        return payload_format(field.offset, 1, boost::str(
                    boost::format(fmt_string) % \
                    (unsigned int) source_id % source_name \
                    % field.comment));
    } else 
        return "Unsupported field type!";
};

std::ostream& DecodedTelegram::TrackInfo::debug_repr(std::ostream& outputStream) {
    return generic_debug_repr(outputStream, this);
}

std::ostream& DecodedTelegram::StatusInfo::debug_repr(std::ostream& outputStream) {
    return generic_debug_repr(outputStream, this);
}

std::ostream& DecodedTelegram::AudioBus::debug_repr(std::ostream& outputStream) {
    std::string analysis;
    debug_field_list fields;
    if(this->tgram_meaning == request_status)
        analysis += payload_header("Audio Bus status inquiry telegram");
    else if (this->tgram_meaning == status_distributing) {
        analysis += payload_header("Audio Bus status reply: Currently distributing");
        if(this->payload.size() == 4) {
            fields = {
                {0, 1, hexbyte, "Unknown"},
                {1, 1, hexbyte, "Unknown"},
                {2, 1, hexbyte, "Unknown"},
                {3, 1, ml_source, "Currently playing source"}
            };
        } else if(this->payload.size() == 5) {
            fields = {
                {0, 1, hexbyte, "Unknown"},
                {1, 1, hexbyte, "Unknown"},
                {2, 1, hexbyte, "Unknown"},
                {3, 1, ml_source, "Currently playing source"},
                {4, 1, hexbyte, "Unknown"},
            };
        }
    } else if (this->tgram_meaning == status_not_distributing) {
        analysis += payload_header("Audio Bus status reply: No bus activity");
    } else if (this->tgram_meaning == status_not_distributing) {
        analysis += payload_warning("Unknown Audio Bus telegram");
    };

    unsigned int i = 0;

    if(!fields.size()) {
        while (i++ < this->payload.size()) {
            fields.push_back({(unsigned int)fields.size(), 1, hexbyte, "Unknown"});
        }
    }
    for(auto x: fields) {
        analysis += format_field(*this, x);
    }
    outputStream << analysis;
    return outputStream;
};

std::ostream& DecodedTelegram::GotoSource::debug_repr(std::ostream& outputStream) {
    std::string analysis;
    analysis += payload_header("");
    int i = 0;
    for(auto x: this->payload) {
        analysis += payload_format(i++, 1, boost::str(boost::format("%|02X|") % \
                    (unsigned int) x));
    }
    outputStream << analysis;
    return outputStream;
};

std::ostream& DecodedTelegram::DisplayData::debug_repr(std::ostream& outputStream) {
    std::string analysis;

    unsigned int i = 0;
    bool surprises = false;

    debug_field_list fields;
    if(this->payload.size() != 17) {
        analysis += payload_header("Display data telegram");
        analysis += payload_warning("Unexpected length of telegram!");
    } else {
        analysis += payload_header("Display data: [" + std::string(this->payload.begin() + 5, this->payload.end() - 2) + "]");
        fields = {
            {0, 1, hexbyte, "Unknown (Always 0x03)"},
            {1, 1, hexbyte, "Unknown (0x01 or 0x02)"},
            {2, 1, hexbyte, "Unknown (Always 0x01)"},
            {3, 1, hexbyte, "Unknown (Always 0x00)"},
            {4, 1, hexbyte, "Unknown (Always 0x00)"},
            {5, 12, fixed_width_ascii, "Display data"},
        };
    }

    for(auto x: fields) {
        analysis += format_field(*this, x);
    }

    std::vector<std::pair<bool, uint8_t>> expectations = {
        {true, 0x03},
        {false, 0x01}, // this is sometimes 0x02
        {true, 0x01},
        {true, 0x00},
        {true, 0x00}
    };

    while(i < expectations.size()) {
        if(this->payload[i] != expectations[i].second) {
            if(expectations[i].first) {
                surprises = true;
                analysis.append("Found unexpected data!\n");
            }
        }
        i++;
    }
    if(surprises) {
        i = 0;
        while(i < expectations.size()) {
            analysis.append((boost::format("%02X: %02X (expected %02X)\n") % i \
                        % (unsigned int)this->payload[i] \
                        % (unsigned int)expectations[i].second).str());
            i++;
        }
    }

    return outputStream << analysis;
};

std::ostream& DecodedTelegram::Metadata::debug_repr(std::ostream& outputStream) {
    return outputStream << "Metadata message: " << this->key << ": \"" << this->value << "\"";
}

std::ostream& DecodedTelegram::MasterPresent::debug_repr(std::ostream& outputStream) {
    return generic_debug_repr(outputStream, this);
}
bool DecodedTelegram::Metadata::any_surprises_here() {
    unsigned int i = 0;
    std::string analysis = "";
    bool anything_unexpected = false;
    while(i < 14) {
        // Do we expect a specific value?
        if(this->expectations[i].first) {
            // Is it the value we're expecting?
            if(this->payload[i] != this->expectations[i].second) {
                anything_unexpected = true;
                analysis.append("\x1b[91m");
                analysis.append(boost::str(boost::format("Expected %02X") % this->expectations[i].second));
            } else {
                analysis.append("\x1b[92m");
            }
        } else {
            analysis.append("\x1b[93m");
        }

        analysis.append(boost::str(boost::format("\t%02d: %02X [%s]\x1b[0m\n") % i % this->payload[i] % labels[i]));
        i++;
    }
    if (anything_unexpected)
        std::cout << analysis;
    return anything_unexpected;
};


// new dump format
// {num_bytes, data_type, expected_value_policy, comment}
// expected_value_policy = std::pair<policy enum, std::vector<uint8_t, n>>
