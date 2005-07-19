#ifndef CELLISTVIEWITEM_H
#define CELLISTVIEWITEM_H

class CelListViewItem : public QListViewItem
{
   
public:
    CelListViewItem( QListView * parent, std::string name, double dist, const char* dist_unit, double app_mag, double abs_mag, QString type);
    CelListViewItem( QListViewItem * parent, std::string name, double dist, const char* dist_unit, double app_mag, double abs_mag, QString type);
    ~CelListViewItem();   
        
    std::string getName() const { return name; }

    double getDist() const { return dist; }
    double getAppMag() const { return app_mag; }
    double getAbsMag() const { return abs_mag; }
    
public slots:
    int compare ( QListViewItem * i, int col, bool ascending ) const;
    
private:
    std::string name;
    double dist;
    const char* dist_unit;
    double app_mag;
    double abs_mag;
    QString type;
};

#endif // CELESTIALBROWSER_H

