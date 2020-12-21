/***
 *         Author:  Dilawar Singh <dilawa@subcom.tech>
 *    Description:  Utility/helper functions.
 */


#ifndef UTIL_H
#define UTIL_H

extern int graphicsreadcolor(char **stringptr,double *rgba);

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  Split a filepath between dirname/filename. For examples::
 *  /a/b/c.txt -> (/a/b, c.txt)
 *
 * @Param filepath
 *
 * @Returns
 */
/* ----------------------------------------------------------------------------*/
inline pair<string, string> splitPath(const string& p)
{
    string filename, fileroot;
    auto   pos = p.find_last_of('/');
    fileroot   = p.substr(0, pos + 1);
    filename   = p.substr(pos + 1);
    return {fileroot, filename};
}

inline array<double, 4> color2RGBA(char* color)
{
    array<double, 4> rgba = {0, 0, 0, 1.0};
    graphicsreadcolor(&color, &rgba[0]);
    return rgba;
}

#endif /* end of include guard: UTIL_H */
