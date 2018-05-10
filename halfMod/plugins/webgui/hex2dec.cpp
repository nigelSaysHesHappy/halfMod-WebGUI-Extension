#include <iostream>
#include <string>
using namespace std;

/*string dec2hex(unsigned int n)
{
    string res;
    do
    {
        res += "0123456789ABCDEF"[n % 16];
        n >>= 4;
    } while (n);
    return string(res.rbegin(), res.rend());
}
string hexify(string in)
{
    string out;
    for (auto it = in.begin(), ite = in.end();it != ite;++it)
        out += dec2hex(int(*it));
    return out;
}*/
string decify(string in)
{
    string out;
    for (int i = 0, j = in.size();i < j;i+=2)
        out += char(stoi(in.substr(i,2),0,16));
    return out;
}
string strreplace(string text, string find, string replace)
{
    int flen = find.size(), rlen = replace.size();
    for (int x = 0; x < int(text.size()); x++)
    {
        if (text.compare(x,flen,find) == 0)
        {
            text.erase(x,flen);
            text.insert(x,replace);
            x+=rlen-1;
        }
    }
    return text;
}
string hex2txt(string in)
{
    string ret;
    
    for (int i = 0;i < in.size();i++)
    {
        if ((in[i] == '%') && (i < in.size()-2))
        {
            ret += char(stoi(in.substr(i+1,2),0,16));
            i+=2;
        }
        else ret += in[i];
    }
    return strreplace(ret,"+"," ");
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        string in = argv[1];
        if (in == "--code")
        {
            if (argc > 2)
            {
                in = argv[2];
                cout<<hex2txt(in)<<endl;
            }
        }
        else
            cout<<decify(in)<<endl;
        //cout<<"Input: "<<in<<endl;
        //cout<<"hexify(input): "<<hexify(in)<<endl;
        //cout<<"decify(hexify(input)): "<<decify(hexify(in))<<endl;
    }
    return 0;
}

