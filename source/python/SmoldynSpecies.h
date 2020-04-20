/***
 *    Description:  Species in smoldyn.
 *
 *        Created:  2020-04-21

 *         Author:  Dilawar Singh <dilawars@ncbs.res.in>
 *   Organization:  NCBS Bangalore
 *        License:  MIT License
 */

#ifndef SMOLDYNSPECIES_H
#define SMOLDYNSPECIES_H

class SmoldynSpecies {
public:
    SmoldynSpecies(const string& name);

    ~SmoldynSpecies();

    void setDiffConst(const double D);
    double getDiffConst(void) const;

    void setColor(const string& color);
    string getColor(void) const;

    void setDisplaySize(const double ds);
    double getDisplaySize() const;

    string getName() const;

private:
    /* data */
    string name_;
    double diffConst_;
    string color_;
    double displaySize_;
};


#endif /* end of include guard: SMOLDYNSPECIES_H */
