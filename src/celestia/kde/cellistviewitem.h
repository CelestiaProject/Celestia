#ifndef CELLISTVIEWITEM_H
#define CELLISTVIEWITEM_H

class CelListViewItem : public QListViewItem
{
   
public:
    CelListViewItem( QListView * parent, QString name, QString label1, QString label2 = QString::null, QString label3 = QString::null, QString label4 = QString::null, QString label5 = QString::null, QString label6 = QString::null, QString label7 = QString::null, QString label8 = QString::null );
    CelListViewItem( QListViewItem * parent, QString name, QString label1, QString label2 = QString::null, QString label3 = QString::null, QString label4 = QString::null, QString label5 = QString::null, QString label6 = QString::null, QString label7 = QString::null, QString label8 = QString::null );
    ~CelListViewItem();   
        
    QString getName() const { return name; }
    
public slots:
    int compare ( QListViewItem * i, int col, bool ascending ) const;
    
private:
    QString name;
};

#endif // CELESTIALBROWSER_H

