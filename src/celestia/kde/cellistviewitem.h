#ifndef CELLISTVIEWITEM_H
#define CELLISTVIEWITEM_H

class CelListViewItem : public QListViewItem
{
   
public:
    CelListViewItem( QListView * parent, QString label1, QString label2 = QString::null, QString label3 = QString::null, QString label4 = QString::null, QString label5 = QString::null, QString label6 = QString::null, QString label7 = QString::null, QString label8 = QString::null );
    CelListViewItem( QListViewItem * parent, QString label1, QString label2 = QString::null, QString label3 = QString::null, QString label4 = QString::null, QString label5 = QString::null, QString label6 = QString::null, QString label7 = QString::null, QString label8 = QString::null );
    ~CelListViewItem();   
        
public slots:
    int compare ( QListViewItem * i, int col, bool ascending ) const;
};

#endif // CELESTIALBROWSER_H

