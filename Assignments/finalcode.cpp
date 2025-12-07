#include <iostream> 
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <regex>
#include <stdexcept>
#include <limits>
#include <unordered_set>
#include <unordered_map>

namespace fs = std::filesystem;

static constexpr int    START_OFFSET_MIN   = 3;  // +3 minutes base for first extra window
static constexpr int    MAX_ATTEMPTS       = 12;
static constexpr double SLIPPAGE           = 0.5;
static constexpr double EPS                = 1e-9;
static constexpr int    OUTPUT_ROW_OFFSET  = 20; // number of blank rows before header

// ───────────────────────────── utils
static inline std::string trim(const std::string& s){
    size_t a = s.find_first_not_of(" \t\r\n\"'");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n\"'");
    return s.substr(a, b - a + 1);
}
static inline std::string tolower_str(std::string s){
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}
static inline std::string norm_alnum(const std::string& s){
    std::string o; o.reserve(s.size());
    for (unsigned char ch : s)
        if (std::isalnum(ch)) o.push_back((char)std::tolower(ch));
    return o;
}
static inline std::string strip_invisible(std::string s){
    std::string out; out.reserve(s.size());
    for (size_t i=0;i<s.size();++i){
        unsigned char c=(unsigned char)s[i];
        if (c==0xEF && i+2<s.size() && (unsigned char)s[i+1]==0xBB && (unsigned char)s[i+2]==0xBF){ i+=2; continue; }
        if (c<32 && c!='\t' && c!=' ') continue;
        out.push_back((char)c);
    }
    return out;
}
static inline bool starts_with_ci_after_trim_ohlcv(const std::string& raw){
    std::string h = strip_invisible(raw);
    size_t i=0;
    while(i<h.size() && (h[i]==' ' || h[i]=='\t' || h[i]=='"' || h[i]=='\'')) ++i;
    std::string tail = tolower_str(h.substr(i));
    return tail.rfind("ohlcv ",0)==0;
}

// Read first non-empty (non-whitespace) line, return false on EOF
static bool getline_nonempty(std::istream& in, std::string& line){
    while (std::getline(in, line)){
        bool any = false;
        for(char c : line){
            if(c!=' ' && c!='\t' && c!='\r' && c!='\n'){
                any = true;
                break;
            }
        }
        if(any) return true;
    }
    return false;
}

static inline std::vector<std::string> splitCSV(const std::string& line){
    std::vector<std::string> out; std::string cur; bool inq=false;
    for(size_t i=0;i<line.size();++i){
        char ch=line[i];
        if(inq){
            if(ch=='"'){
                if(i+1<line.size() && line[i+1]=='"'){ cur.push_back('"'); ++i; }
                else inq=false;
            } else cur.push_back(ch);
        }else{
            if(ch=='"') inq=true;
            else if(ch==','){ out.push_back(trim(cur)); cur.clear(); }
            else cur.push_back(ch);
        }
    }
    out.push_back(trim(cur));
    return out;
}
static inline double safe_stod(const std::string& s){
    try { return std::stod(s); } catch (...) { return NAN; }
}

static void writeCSV_raw(const std::string& filename,
                         const std::vector<std::string>& headers,
                         const std::vector<std::vector<std::string>>& rows){
    std::ofstream out(filename);
    if(!out){
        std::cerr<<"❌ Cannot open "<<filename<<"\n";
        return;
    }

    auto quote=[&](const std::string& s){
        std::string t=s;
        for(size_t pos=0;(pos=t.find('"',pos))!=std::string::npos;pos+=2)
            t.insert(pos,"\"");
        return "\""+t+"\"";
    };

    // Push everything down by OUTPUT_ROW_OFFSET rows
    for(int i = 0; i < OUTPUT_ROW_OFFSET; ++i){
        out << "\n";
    }

    // header row
    for(size_t i=0;i<headers.size();++i){
        out<<quote(headers[i]);
        if(i+1<headers.size()) out<<",";
    }
    out<<"\n";

    // data rows
    for(const auto& r: rows){
        for(size_t i=0;i<r.size();++i){
            out<<quote(r[i]);
            if(i+1<r.size()) out<<",";
        }
        out<<"\n";
    }

    std::cout<<"✅ Wrote "<<rows.size()<<" rows → "<<filename<<"\n";
}

static int findColByNames(const std::vector<std::string>& H, const std::vector<std::string>& cands){
    std::vector<std::string> N(H.size());
    for(size_t i=0;i<H.size();++i) N[i]=norm_alnum(H[i]);
    for(const auto& s: cands){
        std::string ns=norm_alnum(s);
        for(size_t i=0;i<N.size();++i)
            if(N[i].find(ns)!=std::string::npos) return (int)i;
    }
    return -1;
}
static int findColByNamesExact(const std::vector<std::string>& H, const std::vector<std::string>& cands){
    std::vector<std::string> N(H.size());
    for(size_t i=0;i<H.size();++i) N[i]=norm_alnum(H[i]);
    for(const auto& s: cands){
        std::string ns=norm_alnum(s);
        for(size_t i=0;i<N.size();++i)
            if(N[i]==ns) return (int)i;
    }
    return -1;
}
static int findColPL_Tight(const std::vector<std::string>& H){
    int i=findColByNamesExact(H, {"P/L","PL","PnL","ProfitLoss"});
    if(i!=-1) return i;
    for(size_t k=0;k<H.size();++k){
        auto n=norm_alnum(H[k]);
        if(n=="pl" || n=="pnl" || n=="profitloss") return (int)k;
    }
    return -1;
}

// ───────────────────────────── ET time helpers
static inline std::time_t timegm_compat(std::tm* t){
#ifdef _WIN32
    return _mkgmtime(t);
#else
    return timegm(t);
#endif
}
static bool parse_flex_ts(const std::string& s, std::tm& t){
    std::string x=trim(s); int Y=0,M=0,D=0,h=0,m=0,sec=0; int n=0;
    n=std::sscanf(x.c_str(), "%d/%d/%d %d:%d:%d", &M,&D,&Y,&h,&m,&sec);
    if(n<5){
        n=std::sscanf(x.c_str(), "%d/%d/%d %d:%d", &M,&D,&Y,&h,&m);
        if(n>=5) sec=0;
    }
    if(n>=5 && M>0 && D>0 && Y>0){
        t={};
        t.tm_year=Y-1900; t.tm_mon=M-1; t.tm_mday=D;
        t.tm_hour=h; t.tm_min=m; t.tm_sec=sec; t.tm_isdst=-1;
        return true;
    }
    n=std::sscanf(x.c_str(), "%d-%d-%d %d:%d:%d", &Y,&M,&D,&h,&m,&sec);
    if(n<5){
        n=std::sscanf(x.c_str(), "%d-%d-%d %d:%d", &Y,&M,&D,&h,&m);
        if(n>=5) sec=0;
    }
    if(n>=5 && M>0 && D>0 && Y>0){
        t={};
        t.tm_year=Y-1900; t.tm_mon=M-1; t.tm_mday=D;
        t.tm_hour=h; t.tm_min=m; t.tm_sec=sec; t.tm_isdst=-1;
        return true;
    }
    return false;
}
static inline std::time_t utc_to_et(std::time_t utc){
    std::tm* g=std::gmtime(&utc);
    int Y=g->tm_year+1900;
    auto nth=[&](int month,int weekday,int n){
        std::tm tm0{}; tm0.tm_year=Y-1900; tm0.tm_mon=month-1; tm0.tm_mday=1;
        std::time_t t0=timegm_compat(&tm0);
        std::tm g0=*gmtime(&t0);
        int first=g0.tm_wday; int delta=(weekday-first+7)%7;
        return 1+delta+7*(n-1);
    };
    std::tm S{}; S.tm_year=Y-1900; S.tm_mon=2; S.tm_mday=nth(3,0,2); S.tm_hour=7;
    std::tm E{}; E.tm_year=Y-1900; E.tm_mon=10;E.tm_mday=nth(11,0,1); E.tm_hour=6;
    std::time_t s=timegm_compat(&S), e=timegm_compat(&E);
    bool dst = (utc>=s && utc<e);
    return utc + (dst?-4:-5)*3600;
}
static inline std::time_t et_epoch_from_et_tm(const std::tm& et){
    std::tm t5=et; t5.tm_isdst=-1; t5.tm_hour+=5;
    std::time_t utc=timegm_compat(&t5);
    return utc_to_et(utc);
}
static bool trade_time_from_filename_ET(const std::string& path,std::time_t& base_et){
    std::string f=fs::path(path).filename().string();
    std::smatch m; std::regex rx(R"((\d{8})_(\d{6}))");
    if(!std::regex_search(f,m,rx)) return false;
    std::string ymd=m[1], hms=m[2];
    int Y=stoi(ymd.substr(0,4)), Mo=stoi(ymd.substr(4,2)), D=stoi(ymd.substr(6,2));
    int h=stoi(hms.substr(0,2)), mi=stoi(hms.substr(2,2)), s=stoi(hms.substr(4,2));
    std::tm t{};
    t.tm_year=Y-1900; t.tm_mon=Mo-1; t.tm_mday=D;
    t.tm_hour=h; t.tm_min=mi; t.tm_sec=s; t.tm_isdst=-1;
    base_et = et_epoch_from_et_tm(t);
    return true;
}
static bool last_et_timestamp_in_csv(const std::string& file,std::time_t& last,std::string& out){
    std::ifstream in(file); if(!in) return false;
    std::string line;
    if(!std::getline(in,line)) return false; // can be blank/header; that’s fine
    bool found=false; std::time_t best=0; std::string bests;
    while(std::getline(in,line)){
        auto cols=splitCSV(line);
        for(const auto& c: cols){
            if((c.find('/')==std::string::npos && c.find('-')==std::string::npos) || c.find(':')==std::string::npos) continue;
            std::tm t{};
            if(!parse_flex_ts(c,t)) continue;
            std::time_t loc=et_epoch_from_et_tm(t);
            if(!found || loc>best){
                found=true; best=loc; bests=trim(c);
            }
        }
    }
    in.close();
    if(!found) return false;
    last=best; out=bests; return true;
}

// ───────────────────────────── name helpers
static inline bool ends_with_ci(const std::string& s, const std::string& suf){
    std::string a=tolower_str(s), b=tolower_str(suf);
    if(b.size()>a.size()) return false;
    return std::equal(b.rbegin(), b.rend(), a.rbegin());
}
static inline bool is_unresolved_name(const std::string& n){ return ends_with_ci(n,"_unresolved.csv"); }
static inline bool is_resolved_name(const std::string& n){ return ends_with_ci(n,"_resolved.csv"); }
static inline bool is_merged_name(const std::string& n){ return tolower_str(n).find("_merged")!=std::string::npos; }
static inline std::string strip_derivative_suffixes(std::string stem){
    static const std::regex rx(R"((?:_(?:Next\d+Min|Merged|Resolved|Unresolved))+$)");
    return std::regex_replace(stem, rx, "");
}
static inline std::string base_key_from_path(const fs::path& p){
    return tolower_str(strip_derivative_suffixes(p.stem().string()));
}

// ───────────────────────────── PT columns (keep order)
struct PTIdx{ int openCol=-1,qCol=-1,rCol=-1,plCol=-1,resCol=-1; };
static PTIdx find_pt_indices(const std::vector<std::string>& H, bool isBuy){
    PTIdx i;
    i.openCol = findColByNamesExact(H, isBuy ? std::vector<std::string>{"Buy Open Price","BuyOpenPrice"}
                                             : std::vector<std::string>{"Sell Open Price","SellOpenPrice"});
    if(i.openCol==-1) i.openCol = findColByNamesExact(H, {"Open Price","OpenPrice"});
    i.qCol   = findColByNamesExact(H, {"Profit Filled","ProfitFilled"});
    i.rCol   = findColByNamesExact(H, {"Stop Filled","StopFilled"});
    i.plCol  = findColPL_Tight(H);
    i.resCol = findColByNamesExact(H, {"Resolved?","Resolved"});
    return i;
}
static void ensure_pt_cols(std::vector<std::string>& H,
                           std::vector<std::vector<std::string>>& rows,
                           bool isBuy, PTIdx& idx){
    auto add=[&](int& col,const std::string& name){
        if(col==-1){
            H.push_back(name);
            col=(int)H.size()-1;
            for(auto& r:rows) r.resize(H.size(),"");
        }
    };
    add(idx.openCol, isBuy? "Buy Open Price":"Sell Open Price");
    add(idx.qCol,    "Profit Filled");
    add(idx.rCol,    "Stop Filled");
    add(idx.plCol,   "P/L");
    add(idx.resCol,  "Resolved?");
}
static bool looks_like_datetime(const std::string& s){
    return (s.find(':')!=std::string::npos) &&
           (s.find('/')!=std::string::npos || s.find('-')!=std::string::npos);
}
static void numeric_only(std::vector<std::vector<std::string>>& rows, int c){
    if(c<0) return;
    for(auto& r: rows){
        if((int)r.size()<=c || r[c].empty()) continue;
        if(looks_like_datetime(r[c])){ r[c]=""; continue; }
        double v=safe_stod(r[c]);
        if(std::isnan(v)) r[c]="";
        else r[c]=std::to_string(v);
    }
}
static void sanitize_pt_values(bool isBuy,const std::vector<std::string>& H,PTIdx idx,
                               std::vector<std::vector<std::string>>& rows){
    numeric_only(rows, idx.openCol);
    numeric_only(rows, idx.qCol);
    numeric_only(rows, idx.rCol);
    numeric_only(rows, idx.plCol);
    for(auto& r: rows){
        if(idx.resCol>=0 && r[idx.resCol].empty()) r[idx.resCol]="Not Resolved";
        if(idx.plCol>=0 && r[idx.plCol].empty()){
            double P = (idx.openCol>=0 && idx.openCol<(int)r.size()) ? safe_stod(r[idx.openCol]) : NAN;
            double Q = (idx.qCol>=0   && idx.qCol<(int)r.size())   ? safe_stod(r[idx.qCol])   : NAN;
            double R = (idx.rCol>=0   && idx.rCol<(int)r.size())   ? safe_stod(r[idx.rCol])   : NAN;
            double F = !std::isnan(Q)? Q : (!std::isnan(R)? R : NAN);
            if(!std::isnan(P) && !std::isnan(F)){
                double pl = isBuy ? (F - P) : (P - F);
                if(std::fabs(pl) > EPS) r[idx.plCol] = std::to_string(pl);
            }
        }
    }
}

// ───────────────────────────── forward fill helpers
static int find_by_synonyms(const std::vector<std::string>& H, const std::vector<std::string>& names){
    for(int i=0;i<(int)H.size();++i){
        std::string hn = norm_alnum(H[i]);
        for(const auto& want: names)
            if(hn == norm_alnum(want)) return i;
    }
    return -1;
}
static void forward_fill_columns(std::vector<std::string>& H,
                                 std::vector<std::vector<std::string>>& rows,
                                 const std::vector<std::vector<std::string>>& groups)
{
    for(const auto& names : groups){
        int c = find_by_synonyms(H, names);
        if(c<0) continue;
        std::string carry;
        for(auto& r : rows){
            if((int)r.size()<=c) r.resize(H.size(),"");
            if(!r[c].empty()) carry=r[c];
            else if(!carry.empty()) r[c]=carry;
        }
    }
}
static void forward_fill_indices(std::vector<std::vector<std::string>>& rows,
                                 const std::vector<int>& cols,
                                 int width)
{
    for(int c : cols){
        if(c<0) continue;
        std::string carry;
        for(auto& r : rows){
            if((int)r.size()<width) r.resize(width,"");
            if(!r[c].empty()) carry=r[c];
            else if(!carry.empty()) r[c]=carry;
        }
    }
}

// ───────────────────────────── sort by time (ts_event ascending)
static int find_ts_col(const std::vector<std::string>& H){
    return find_by_synonyms(H, {"ts_event","timestamp","datetime","time","ts","date"});
}
static std::time_t parse_et_from_cell(const std::string& s, bool& ok){
    std::tm t{};
    if(!parse_flex_ts(s,t)){ ok=false; return 0; }
    ok=true; return et_epoch_from_et_tm(t);
}
static void sort_rows_by_ts(std::vector<std::string>& H,
                            std::vector<std::vector<std::string>>& rows)
{
    int tcol = find_ts_col(H);
    if(tcol<0) return; // nothing to sort by
    std::stable_sort(rows.begin(), rows.end(), [&](const auto& a, const auto& b){
        bool oka=false, okb=false;
        std::time_t ta = (tcol<(int)a.size()) ? parse_et_from_cell(a[tcol], oka) : 0;
        std::time_t tb = (tcol<(int)b.size()) ? parse_et_from_cell(b[tcol], okb) : 0;
        if(oka && okb) return ta < tb;
        if(oka != okb) return oka; // rows with valid time first
        return false; // keep order
    });
}

// ───────────────────────────── resolver (order preserved, STOP > STOP-LIMIT)
struct ResolveResult{
    bool   filled=false;
    int    open_idx=-1, fill_idx=-1;
    double open_price=NAN, fill_price=NAN, pl=NAN;
    bool   profit_hit=false;
};

static int find_any(const std::vector<std::string>& H, const std::vector<std::string>& syn){
    int i = findColByNamesExact(H, syn);
    if(i!=-1) return i;
    return findColByNames(H, syn);
}

static double to_number(const std::string& s){ return safe_stod(s); }

static ResolveResult resolve_rows(bool isBuy,
                                  std::vector<std::string>& H,
                                  std::vector<std::vector<std::string>>& rows)
{
    int hi = find_any(H, {"high"});
    int lo = find_any(H, {"low"});
    int tp = find_any(H, {"profit order","profitorder","takeprofit","tp","target","profit","profittarget","takeprofitprice"});

    // Entry STOP preferred; STOP-LIMIT fallback
    int st_stop = isBuy
        ? find_any(H, {"buy stop","buy stop $","buystop","entrybuy","buy"})
        : find_any(H, {"sell stop","sell stop $","sellstop","entrysell","sell"});
    int st_limit = isBuy
        ? find_any(H, {"buy stop limit $","buystoplimit","buystoplimit$"})
        : find_any(H, {"sell stop limit $","sellstoplimit","sellstoplimit$"});

    // Stop-loss STOP preferred; LIMIT fallback
    int sl_stop  = find_any(H, {"stop loss stop $","stoplossstop","stop loss stop","sl stop"});
    int sl_limit = find_any(H, {"stop loss limit $","stoplosslimit","stop loss limit","sl limit"});

    if(hi==-1||lo==-1||tp==-1||(st_stop==-1 && st_limit==-1) || (sl_stop==-1 && sl_limit==-1))
        return {};

    PTIdx idx = find_pt_indices(H, isBuy);
    ensure_pt_cols(H, rows, isBuy, idx);

    double stop=NAN, profit=NAN, loss=NAN;

    // Profit target
    for(const auto& r: rows){
        if(tp>=0 && tp<(int)r.size() && std::isnan(profit)){
            double v = to_number(r[tp]); if(!std::isnan(v)) profit = v;
        }
        if(!std::isnan(profit)) break;
    }
    // Entry STOP preferred; then STOP-LIMIT
    for(const auto& r: rows){
        if(st_stop>=0 && st_stop<(int)r.size() && std::isnan(stop)){
            double v = to_number(r[st_stop]); if(!std::isnan(v)) stop = v;
        }
        if(std::isnan(stop) && st_limit>=0 && st_limit<(int)r.size()){
            double v = to_number(r[st_limit]); if(!std::isnan(v)) stop = v;
        }
        if(!std::isnan(stop)) break;
    }
    // Stop-loss STOP preferred; then LIMIT
    for(const auto& r: rows){
        if(sl_stop>=0 && sl_stop<(int)r.size() && std::isnan(loss)){
            double v = to_number(r[sl_stop]); if(!std::isnan(v)) loss = v;
        }
        if(std::isnan(loss) && sl_limit>=0 && sl_limit<(int)r.size()){
            double v = to_number(r[sl_limit]); if(!std::isnan(v)) loss = v;
        }
        if(!std::isnan(loss)) break;
    }

    if(std::isnan(stop) || std::isnan(profit) || std::isnan(loss)) return {};

    ResolveResult rr{};
    for(int i=0;i<(int)rows.size();++i){
        const auto& r=rows[i];
        double hi_=safe_stod(r[hi]), lo_=safe_stod(r[lo]);
        if(isBuy){
            if(rr.open_idx==-1){
                if(!std::isnan(hi_) && hi_>=stop){
                    rr.open_idx=i; rr.open_price=stop+SLIPPAGE;
                }
            } else if(rr.fill_idx==-1){
                if(!std::isnan(hi_) && hi_>=profit){
                    rr.fill_idx=i; rr.profit_hit=true; rr.fill_price=profit-SLIPPAGE; break;
                }
                if(!std::isnan(lo_) && lo_<=loss){
                    rr.fill_idx=i; rr.profit_hit=false; rr.fill_price=loss-SLIPPAGE; break;
                }
            }
        }else{
            if(rr.open_idx==-1){
                if(!std::isnan(lo_) && lo_<=stop){
                    rr.open_idx=i; rr.open_price=stop-SLIPPAGE;
                }
            } else if(rr.fill_idx==-1){
                if(!std::isnan(lo_) && lo_<=profit){
                    rr.fill_idx=i; rr.profit_hit=true; rr.fill_price=profit+SLIPPAGE; break;
                }
                if(!std::isnan(hi_) && hi_>=loss){
                    rr.fill_idx=i; rr.profit_hit=false; rr.fill_price=loss+SLIPPAGE; break;
                }
            }
        }
    }

    if(!std::isnan(rr.open_price) && !std::isnan(rr.fill_price))
        rr.pl = isBuy ? (rr.fill_price - rr.open_price) : (rr.open_price - rr.fill_price);

    bool saw_first=false;
    for(int i=0;i<(int)rows.size();++i){
        auto& r=rows[i];
        if(idx.resCol>=0 && r[idx.resCol].empty()) r[idx.resCol]="Not Resolved";
        if(i==rr.open_idx && idx.openCol>=0 && r[idx.openCol].empty())
            r[idx.openCol]=std::to_string(rr.open_price);

        if(i==rr.fill_idx){
            if(rr.profit_hit && idx.qCol>=0 && r[idx.qCol].empty())
                r[idx.qCol]=std::to_string(rr.fill_price);
            if(!rr.profit_hit && idx.rCol>=0 && r[idx.rCol].empty())
                r[idx.rCol]=std::to_string(rr.fill_price);
            if(idx.plCol>=0 && r[idx.plCol].empty() && !std::isnan(rr.pl) && std::fabs(rr.pl)>EPS)
                r[idx.plCol]=std::to_string(rr.pl);
            if(idx.resCol>=0)
                r[idx.resCol] = saw_first ? "Resolved (Before)" : "Resolved (First)";
            saw_first=true;
        } else if(saw_first && idx.resCol>=0){
            r[idx.resCol]="Resolved (Before)";
        }
    }

    rr.filled = (rr.fill_idx!=-1);
    sanitize_pt_values(isBuy, H, idx, rows);
    return rr;
}

// ───────────────────────────── ID/name normalization (in-place)
static inline int find_col_exact(const std::vector<std::string>& H, const std::string& label){
    for (int i=0;i<(int)H.size();++i)
        if (H[i] == label) return i;
    return -1;
}
static void normalize_id_name_inplace(std::vector<std::string>& H,
                                      std::vector<std::vector<std::string>>& rows){
    auto move_numeric = [&](const char* nameCol, const char* idCol){
        int cName=find_col_exact(H, nameCol), cID=find_col_exact(H, idCol);
        if(cName==-1 || cID==-1) return;
        for(auto& r: rows){
            if((int)r.size()<=std::max(cName,cID)) continue;
            const std::string& v=r[cName];
            if(v.empty()) continue;
            char* end=nullptr;
            std::strtod(v.c_str(), &end);
            if(end && *end=='\0'){
                if(r[cID].empty()) r[cID]=v;
                r[cName].clear();
            }
        }
    };
    move_numeric("Publisher",  "Publisher ID");
    move_numeric("Instrument", "Instrument ID");
}

// ───────────────────────────── Merge (STRICT, canonicalized, sorted)
static bool canonicalize_right_header(const std::string& raw, std::string& keyOut, std::string& labelOut){
    std::string k = norm_alnum(strip_invisible(raw));
    if(k.rfind("ohlcv",0)==0) k.erase(0,5);

    auto is_time_key=[&](const std::string& s){
        return (s=="tsevent"||s=="timestamp"||s=="datetime"||s=="date"||s=="time"||s=="ts");
    };
    auto is_ohlcv_key=[&](const std::string& s){
        return (s=="open"||s=="high"||s=="low"||s=="close"||s=="volume");
    };

    if(is_time_key(k)){ keyOut="tsevent";  labelOut="ts_event"; return true; }
    if(is_ohlcv_key(k)){ keyOut=k;         labelOut=(k=="open"?"open":k); return true; }
    if(k=="rtype"||k=="type"){ keyOut="rtype"; labelOut="rtype"; return true; }
    if(k=="publisherid"||k=="publisher"){ keyOut="publisherid"; labelOut="Publisher"; return true; }
    if(k=="instrumentid"||k=="instrument"){ keyOut="instrumentid"; labelOut="Instrument"; return true; }
    if(k=="symbol"){ keyOut="symbol"; labelOut="symbol"; return true; }
    return false;
}

static void mergeCSVs_union(const std::string& fLeft,
                            const std::string& fOHLCV,
                            const std::string& outMerged)
{
    std::ifstream a(fLeft), b(fOHLCV);
    if(!a||!b) throw std::runtime_error("Missing merge inputs");

    std::string hL, hR;
    if(!getline_nonempty(a,hL) || !getline_nonempty(b,hR)) return;
    auto leftH_raw  = splitCSV(hL);
    auto rightH_raw = splitCSV(hR);

    // LEFT FILTER: drop any "OHLCV ..." column that leaked
    std::vector<std::string> leftH;
    leftH.reserve(leftH_raw.size());
    for(int i=0;i<(int)leftH_raw.size();++i){
        if(starts_with_ci_after_trim_ohlcv(leftH_raw[i])) continue;
        leftH.push_back(leftH_raw[i]);
    }

    // RIGHT FILTER + CANONICALIZATION
    struct RMap { int src; std::string key; std::string label; };
    std::vector<RMap> rightCols;
    for(int i=0;i<(int)rightH_raw.size();++i){
        std::string key,label;
        if(canonicalize_right_header(rightH_raw[i], key, label)){
            rightCols.push_back({i,key,label});
        }
    }

    // Build union header: put time first
    std::vector<std::string> H;
    int leftTs = find_by_synonyms(leftH, {"ts_event","timestamp","datetime","time","ts"});
    if(leftTs==-1){
        bool rightHasTs=false;
        for(const auto& rc: rightCols)
            if(rc.key=="tsevent"){ rightHasTs=true; break; }
        if(rightHasTs) H.push_back("ts_event");
    }else{
        H.push_back(leftH[leftTs]);
    }
    for(size_t i=0;i<leftH.size();++i){
        if((int)i==leftTs) continue;
        H.push_back(leftH[i]);
    }
    auto has_meaning = [&](const std::vector<std::string>& names){
        return find_by_synonyms(H, names) != -1;
    };
    auto add_if_missing = [&](const std::vector<std::string>& names, const std::string& label){
        if(!has_meaning(names)) H.push_back(label);
    };
    add_if_missing({"open"}, "open");
    add_if_missing({"high"}, "high");
    add_if_missing({"low"},  "low");
    add_if_missing({"close"},"close");
    add_if_missing({"volume"},"volume");
    add_if_missing({"rtype"}, "rtype");
    add_if_missing({"publisher id","publisher"}, "Publisher");
    add_if_missing({"instrument id","instrument"}, "Instrument");
    add_if_missing({"symbol"}, "symbol");

    // Map LEFT columns by exact names present in leftH_raw
    std::vector<int> mapL(H.size(), -1);
    for(size_t j=0;j<H.size();++j){
        std::string target = H[j];
        int src = find_by_synonyms(leftH, {target});
        if(src==-1 && norm_alnum(target)=="tsevent")
            src = find_by_synonyms(leftH, {"ts_event","timestamp","datetime","time","ts"});
        if(src!=-1){
            for(size_t k=0;k<leftH_raw.size();++k)
                if(leftH_raw[k]==leftH[src]){ mapL[j]=(int)k; break; }
        }
    }

    // Map RIGHT
    std::vector<int> mapR(H.size(), -1);
    auto dest_for_key = [&](const std::string& key)->int{
        if(key=="tsevent")     return find_by_synonyms(H, {"ts_event","timestamp","datetime","time","ts"});
        if(key=="open")        return find_by_synonyms(H, {"open"});
        if(key=="high")        return find_by_synonyms(H, {"high"});
        if(key=="low")         return find_by_synonyms(H, {"low"});
        if(key=="close")       return find_by_synonyms(H, {"close"});
        if(key=="volume")      return find_by_synonyms(H, {"volume"});
        if(key=="rtype")       return find_by_synonyms(H, {"rtype"});
        if(key=="publisherid") return find_by_synonyms(H, {"publisher id","publisher"});
        if(key=="instrumentid")return find_by_synonyms(H, {"instrument id","instrument"});
        if(key=="symbol")      return find_by_synonyms(H, {"symbol"});
        return -1;
    };
    for(const auto& rc: rightCols){
        int dj = dest_for_key(rc.key);
        if(dj!=-1) mapR[dj] = rc.src;
    }

    std::vector<std::vector<std::string>> rows;

    // left rows
    {
        a.clear(); a.seekg(0);
        std::string dummy;
        if(!getline_nonempty(a,dummy)) return;
        std::string line;
        while(std::getline(a,line)){
            if(line.empty()) continue;
            auto c=splitCSV(line);
            std::vector<std::string> row(H.size(), "");
            for(size_t j=0;j<H.size();++j){
                int srcCol = mapL[j];
                if(srcCol!=-1 && (size_t)srcCol<c.size()) row[j]=c[srcCol];
            }
            rows.push_back(std::move(row));
        }
    }

    // right rows
    {
        b.clear(); b.seekg(0);
        std::string dummy;
        if(!getline_nonempty(b,dummy)) return;
        std::string line;
        while(std::getline(b,line)){
            if(line.empty()) continue;
            auto c=splitCSV(line);
            std::vector<std::string> row(H.size(), "");
            for(size_t j=0;j<H.size();++j){
                int srcCol = mapR[j];
                if(srcCol!=-1 && (size_t)srcCol<c.size()) row[j]=c[srcCol];
            }
            rows.push_back(std::move(row));
        }
    }

    // Ensure first header is named "ts_event" if it's a time field
    if(!H.empty()){
        std::string h0n = norm_alnum(H[0]);
        if(h0n=="timestamp"||h0n=="datetime"||h0n=="time"||h0n=="ts") H[0]="ts_event";
    }

    // Sort ascending by ts_event BEFORE forward-filling
    sort_rows_by_ts(H, rows);

    // Forward-fill meta + trade parameter columns across sorted rows
    forward_fill_columns(H, rows, {
        {"rtype"},
        {"publisher id","publisher"},
        {"instrument id","instrument"},
        {"symbol"},
        // trade params (both sides)
        {"buy stop"}, {"buy stop $"}, {"buy stop limit $"},
        {"sell stop"}, {"sell stop $"}, {"sell stop limit $"},
        {"profit order"}, {"takeprofit","tp","profittarget","takeprofitprice"},
        {"stop loss stop $","stoplossstop"},
        {"stop loss limit $","stoplosslimit"}
    });

    writeCSV_raw(outMerged, H, rows);
    std::cout<<"✅ Strict, sorted merge completed → "<<outMerged<<"\n";
}

// ───────────────────────────── writer wrapper
static void writeCSV(const std::string& filename,
                     std::vector<std::string> H,
                     std::vector<std::vector<std::string>> rows)
{
    for(auto& r: rows) r.resize(H.size());
    writeCSV_raw(filename, H, rows);
}

// ───────────────────────────── resolve-only pipeline (Attempt 1)
static void resolve_only_pipeline(const std::string& leftUnresolved,
                                  const std::string& outDir)
{
    std::ifstream in(leftUnresolved);
    if(!in.is_open()) return;
    std::string head;
    if(!getline_nonempty(in, head)){ in.close(); return; }
    auto H=splitCSV(head);

    std::vector<std::vector<std::string>> rows;
    std::string line;
    while(std::getline(in,line)) if(!line.empty()) rows.push_back(splitCSV(line));
    in.close();

    // Normalize + sort by time (if present)
    normalize_id_name_inplace(H, rows);
    sort_rows_by_ts(H, rows);

    // Forward-fill meta + trade params inside the trigger rows
    forward_fill_columns(H, rows, {
        {"rtype"},
        {"publisher id","publisher"},
        {"instrument id","instrument"},
        {"symbol"},
        {"buy stop"}, {"buy stop $"}, {"buy stop limit $"},
        {"sell stop"}, {"sell stop $"}, {"sell stop limit $"},
        {"profit order"}, {"takeprofit","tp","profittarget","takeprofitprice"},
        {"stop loss stop $","stoplossstop"},
        {"stop loss limit $","stoplosslimit"}
    });

    bool isBuy = tolower_str(leftUnresolved).find("buy")!=std::string::npos;
    bool isSell= tolower_str(leftUnresolved).find("sell")!=std::string::npos;
    if(!isBuy && !isSell){
        std::cerr<<"⚠️ Cannot infer side for "<<leftUnresolved<<"\n";
        return;
    }

    PTIdx idx = find_pt_indices(H, isBuy);
    ensure_pt_cols(H, rows, isBuy, idx);

    auto rr = resolve_rows(isBuy, H, rows);

    // Drag-fill PT columns after resolution (Open / ProfitFilled / StopFilled / P&L)
    PTIdx idx2 = find_pt_indices(H, isBuy);
    forward_fill_indices(rows, {idx2.openCol, idx2.qCol, idx2.rCol, idx2.plCol}, (int)H.size());

    std::string out = (fs::path(outDir)/(strip_derivative_suffixes(fs::path(leftUnresolved).stem().string()) +
                                          (rr.filled? "_Resolved.csv":"_Unresolved.csv"))).string();
    normalize_id_name_inplace(H, rows);
    writeCSV(out, H, rows);
}

// ───────────────────────────── merge+resolve pipeline (Attempts 2+)
static void union_merge_and_resolve(const std::string& leftUnresolved,
                                    const std::string& winPath,
                                    const std::string& outDir)
{
    std::string baseStem = strip_derivative_suffixes(fs::path(leftUnresolved).stem().string());
    std::string merged   = (fs::path(outDir)/(baseStem+"_Merged.csv")).string();

    mergeCSVs_union(leftUnresolved, winPath, merged);

    // load merged
    std::ifstream in(merged);
    if(!in.is_open()) return;
    std::string head;
    if(!getline_nonempty(in, head)){ in.close(); return; }
    auto H=splitCSV(head);

    std::vector<std::vector<std::string>> rows;
    std::string line;
    while(std::getline(in,line)) if(!line.empty()) rows.push_back(splitCSV(line));
    in.close();

    // normalize + ensure chronological order
    normalize_id_name_inplace(H, rows);
    sort_rows_by_ts(H, rows);

    // forward-fill again post-merge (sorted rows)
    forward_fill_columns(H, rows, {
        {"rtype"},
        {"publisher id","publisher"},
        {"instrument id","instrument"},
        {"symbol"},
        {"buy stop"}, {"buy stop $"}, {"buy stop limit $"},
        {"sell stop"}, {"sell stop $"}, {"sell stop limit $"},
        {"profit order"}, {"takeprofit","tp","profittarget","takeprofitprice"},
        {"stop loss stop $","stoplossstop"},
        {"stop loss limit $","stoplosslimit"}
    });
    writeCSV(merged, H, rows); // keep merged as-is

    bool isBuy = tolower_str(merged).find("buy")!=std::string::npos;
    bool isSell= tolower_str(merged).find("sell")!=std::string::npos;
    if(!isBuy && !isSell){
        std::cerr<<"⚠️ Cannot infer side for "<<merged<<"\n";
        return;
    }

    PTIdx idx = find_pt_indices(H, isBuy);
    ensure_pt_cols(H, rows, isBuy, idx);

    auto rr = resolve_rows(isBuy, H, rows);

    // Drag-fill PT columns after resolution
    PTIdx idx2 = find_pt_indices(H, isBuy);
    forward_fill_indices(rows, {idx2.openCol, idx2.qCol, idx2.rCol, idx2.plCol}, (int)H.size());

    std::string out = merged.substr(0, merged.size()-4) +
                      (rr.filled? "_Resolved.csv":"_Unresolved.csv");
    normalize_id_name_inplace(H, rows);
    writeCSV(out, H, rows);

    // If resolved, prune sibling unresolved for same base
    const std::string baseKey=base_key_from_path(leftUnresolved);
    if(rr.filled){
        for(auto& f: fs::directory_iterator(outDir)){
            if(!f.is_regular_file()) continue;
            std::string fn=f.path().filename().string();
            if(is_unresolved_name(fn) && base_key_from_path(f.path())==baseKey){
                std::error_code ec; fs::remove(f.path(), ec);
            }
        }
    }else{
        if(!is_merged_name(fs::path(leftUnresolved).filename().string())){
            std::error_code ec; fs::remove(leftUnresolved, ec);
        }
    }
}

// ───────────────────────────── window schedule (+3 min per attempt, attempts ≥ 2)
static int end_off_for_attempt(int attempt){
    if(attempt<=2) return 5;                 // attempt 2: +3..5
    return 5 + 3*(attempt-2);                // attempt 3: +6..8, attempt 4: +9..11, etc.
}

// ───────────────────────────── attempt loop
static void attempt_process(int attempt,
                            const std::string& inDir,
                            const std::string& outDir,
                            const std::string& ohlcvPath)
{
    fs::create_directories(outDir);
    bool first=(attempt==1);

    for(auto& e: fs::directory_iterator(inDir)){
        if(!e.is_regular_file() || e.path().extension()!=".csv") continue;
        const std::string name=e.path().filename().string();
        const std::string lname=tolower_str(name);

        if(first){
            // ONLY raw triggers; no merging on attempt 1
            if(lname.find("_merged")!=std::string::npos || lname.find("_next")!=std::string::npos ||
               lname.find("_resolved")!=std::string::npos || lname.find("_unresolved")!=std::string::npos)
                continue;

            // copy raw → *_Unresolved.csv
            std::ifstream in(e.path());
            if(!in) continue;
            std::string head;
            if(!std::getline(in, head)){ in.close(); continue; }
            auto H=splitCSV(head);
            std::vector<std::vector<std::string>> rows;
            std::string line;
            while(std::getline(in,line)) if(!line.empty()) rows.push_back(splitCSV(line));
            in.close();

            std::string outUnres=(fs::path(outDir)/(e.path().stem().string()+"_Unresolved.csv")).string();
            writeCSV(outUnres,H,rows);

            // resolve-only on attempt 1
            resolve_only_pipeline(outUnres, outDir);
            continue;
        }

        // later attempts: process *_Unresolved.csv only
        if(!is_unresolved_name(name)) continue;

        // window bounds from base time (filename or last timestamp)
        std::time_t base_et{};
        if(!trade_time_from_filename_ET(name, base_et)){
            std::string dummy;
            if(!last_et_timestamp_in_csv(e.path().string(), base_et, dummy)){
                std::cerr<<"⚠️ No trade time for "<<name<<"\n";
                continue;
            }
        }
        int end_off = end_off_for_attempt(attempt);
        bool mergedUnresolved = (lname.find("_merged")!=std::string::npos);
        std::time_t start_et, end_et;
        if(mergedUnresolved){
            int prev_end = end_off_for_attempt(attempt-1);
            start_et = base_et + (prev_end+1)*60;
            end_et   = base_et + end_off*60;
        }else{
            start_et = base_et + START_OFFSET_MIN*60; // +3
            end_et   = base_et + end_off*60;          // attempt 2 → +5; attempt 3 → +8; ...
        }

        // Identify the timestamp column in the OHLCV file robustly
        std::ifstream headIn(ohlcvPath);
        if(!headIn){
            std::cerr<<"❌ OHLCV missing\n";
            continue;
        }
        std::string hdr;
        if(!getline_nonempty(headIn,hdr)){
            std::cerr<<"❌ OHLCV empty\n";
            continue;
        }
        auto oH = splitCSV(hdr);
        int ts_idx = -1;
        for(int i=0;i<(int)oH.size();++i){
            std::string k = norm_alnum(oH[i]);
            if(k.rfind("ohlcv",0)==0) k.erase(0,5);
            if(k=="tsevent"||k=="timestamp"||k=="datetime"||k=="date"||k=="time"||k=="ts"){
                ts_idx=i; break;
            }
        }
        if(ts_idx==-1) ts_idx = 0; // fall back
        headIn.close();

        // write window
        std::ifstream fin(ohlcvPath);
        if(!fin) continue;
        std::string line;
        std::string winPath=(fs::path(outDir)/(strip_derivative_suffixes(e.path().stem().string())+
                             "_Next"+std::to_string(end_off)+"Min.csv")).string();
        std::ofstream fout(winPath);
        if(!fout){
            std::cerr<<"❌ Cannot write "<<winPath<<"\n";
            continue;
        }
        fout<<hdr<<"\n";
        while(std::getline(fin,line)){
            auto c=splitCSV(line);
            if(c.empty()) continue;
            if((int)c.size()<=ts_idx) continue;
            std::tm t{};
            if(!parse_flex_ts(c[ts_idx], t)) continue;
            std::time_t ts_et=et_epoch_from_et_tm(t);
            if(ts_et>=start_et && ts_et<=end_et) fout<<line<<"\n";
        }
        fin.close();
        fout.close();

        // merge + resolve
        union_merge_and_resolve(e.path().string(), winPath, outDir);
    }
}

// ───────────────────────────── main
int main(){
    try{
        // UPDATE paths
        std::string triggerDir = "C:/Users/dedhi/OneDrive/Desktop/Project/Trigger_Windows/";
        std::string outRoot    = "C:/Users/dedhi/OneDrive/Desktop/Project/Resolved_Trades_Attempt/";
        std::string ohlcvPath  = "C:/Users/dedhi/OneDrive/Desktop/Project/OHLCV_1s_Data.csv";

        fs::create_directories(outRoot);

        // Attempt 1: seed unresolved from raw triggers and resolve WITHOUT merging
        int attempt=1;
        std::string attemptDir=(fs::path(outRoot)/("Attempt_"+std::to_string(attempt))).string();
        fs::create_directories(attemptDir);

        bool any_raw=false;
        for(auto& e: fs::directory_iterator(triggerDir)){
            if(!e.is_regular_file() || e.path().extension()!=".csv") continue;
            std::string n=tolower_str(e.path().filename().string());
            if(n.find("_merged")!=std::string::npos || n.find("_next")!=std::string::npos ||
               n.find("_resolved")!=std::string::npos || n.find("_unresolved")!=std::string::npos)
                continue;
            any_raw=true;
            fs::copy_file(e.path(), fs::path(attemptDir)/e.path().filename(),
                          fs::copy_options::overwrite_existing);
        }

        if(!any_raw){
            std::cout<<"⚠️ Attempt 1 found no bare trigger files in Trigger_Windows. It will do nothing this round.\n";
        }

        std::cout<<"\n=========== Attempt "<<attempt<<" ==========="<<std::endl;
        attempt_process(attempt, attemptDir, attemptDir, ohlcvPath);

        while(attempt<MAX_ATTEMPTS){
            int nextAttempt=attempt+1;
            std::string nextDir=(fs::path(outRoot)/("Attempt_"+std::to_string(nextAttempt))).string();
            fs::create_directories(nextDir);

            // carry-forward latest unresolved (prefer merged) with no resolved sibling
            {
                std::unordered_set<std::string> resolved_bases;
                std::unordered_map<std::string, std::vector<fs::path>> unresolved_by_base;
                for(auto& e: fs::directory_iterator(attemptDir)){
                    if(!e.is_regular_file()) continue;
                    const std::string name=e.path().filename().string();
                    const std::string base=base_key_from_path(e.path());
                    if(is_resolved_name(name)) resolved_bases.insert(base);
                    else if(is_unresolved_name(name)) unresolved_by_base[base].push_back(e.path());
                }
                for(auto& [base,paths]: unresolved_by_base){
                    if(resolved_bases.count(base)) continue;
                    fs::path* pick=nullptr;
                    for(auto& p: paths)
                        if(is_merged_name(p.filename().string())){ pick=&p; break; }
                    if(!pick && !paths.empty()) pick=&paths.front();
                    if(!pick) continue;
                    fs::copy_file(*pick, fs::path(nextDir)/pick->filename(),
                                  fs::copy_options::overwrite_existing);
                }
            }

            bool inputs=false;
            for(auto& e: fs::directory_iterator(nextDir))
                if(e.is_regular_file() && is_unresolved_name(e.path().filename().string())){
                    inputs=true; break;
                }
            if(!inputs){
                std::cout<<"\n🎯 Done after "<<attempt<<" attempt(s). Nothing further to process.\n";
                break;
            }

            std::cout<<"\n=========== Attempt "<<nextAttempt<<" ==========="<<std::endl;
            attempt_process(nextAttempt, nextDir, nextDir, ohlcvPath);

            bool any_unresolved=false;
            {
                std::unordered_set<std::string> resB, unresB;
                for(auto& e: fs::directory_iterator(nextDir)){
                    if(!e.is_regular_file()) continue;
                    const std::string name=e.path().filename().string();
                    const std::string base=base_key_from_path(e.path());
                    if(is_resolved_name(name))   resB.insert(base);
                    if(is_unresolved_name(name)) unresB.insert(base);
                }
                for(const auto& b: unresB)
                    if(!resB.count(b)) { any_unresolved=true; break; }
            }
            attempt=nextAttempt;
            attemptDir=nextDir;
            if(!any_unresolved){
                std::cout<<"\n🎯 Done after "<<attempt<<" attempt(s). All resolved or no more inputs.\n";
                break;
            }
        }
        if(attempt>=MAX_ATTEMPTS)
            std::cout<<"\n⚠️ Reached MAX_ATTEMPTS ("<<MAX_ATTEMPTS<<"). Some trades may remain unresolved.\n";

    }catch(const std::exception& e){
        std::cerr<<"❌ Error: "<<e.what()<<"\n";
        return 1;
    }
    return 0;
}
