#include "transactionview.h"

#include "transactionfilterproxy.h"
#include "transactionrecord.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "transactiontablemodel.h"
#include "bitcoinunits.h"
#include "csvmodelwriter.h"
#include "transactiondescdialog.h"
#include "editaddressdialog.h"
#include "optionsmodel.h"
#include "guiutil.h"
#include "thememanager.h"

#include <QScrollBar>
#include <QComboBox>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QTableView>
#include <QHeaderView>
#include <QPushButton>
#include <QMessageBox>
#include <QPoint>
#include <QMenu>
#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QFont>
#include <QDateTimeEdit>

extern ThemeManager *themeManager;
QLabel *pageTitleLabel;

TransactionView::TransactionView(QWidget *parent) :
        QWidget(parent), model(0), transactionProxyModel(0),
        transactionView(0)
{

    //Adding the Page Title QLabel
    pageTitleLabel = new QLabel(tr("Transactions"));
    pageTitleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    pageTitleLabel->setFixedHeight(59);
    pageTitleLabel->setStyleSheet(themeManager->getCurrent()->getMainHeaderStyle());

    // Build filter row
    setContentsMargins(0,0,0,0);

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(0,0,0,0);
#ifdef Q_OS_MAC
    hlayout->setSpacing(5);
    //hlayout->addSpacing(26);
#else
    hlayout->setSpacing(0);
    //hlayout->addSpacing(23);
#endif

    dateWidget = new QComboBox(this);
#ifdef Q_OS_MAC
    dateWidget->setFixedWidth(121);
#else
    dateWidget->setFixedWidth(135);
#endif
    dateWidget->addItem(tr("All"), All);
    dateWidget->addItem(tr("Today"), Today);
    dateWidget->addItem(tr("This week"), ThisWeek);
    dateWidget->addItem(tr("This month"), ThisMonth);
    dateWidget->addItem(tr("Last month"), LastMonth);
    dateWidget->addItem(tr("This year"), ThisYear);
    dateWidget->addItem(tr("Range..."), Range);
    dateWidget->setStyleSheet(themeManager->getCurrent()->getQComboboxTransactionsFilteringStyle());

    hlayout->addWidget(dateWidget);
    hlayout->insertSpacing(1, 8);

    typeWidget = new QComboBox(this);
#ifdef Q_OS_MAC
    typeWidget->setFixedWidth(121);
#else
    typeWidget->setFixedWidth(112);
#endif

    typeWidget->addItem(tr("All"), TransactionFilterProxy::ALL_TYPES);
    typeWidget->addItem(tr("Received with"), TransactionFilterProxy::TYPE(TransactionRecord::RecvWithAddress) |
                                             TransactionFilterProxy::TYPE(TransactionRecord::RecvFromOther));
    typeWidget->addItem(tr("Sent to"), TransactionFilterProxy::TYPE(TransactionRecord::SendToAddress) |
                                       TransactionFilterProxy::TYPE(TransactionRecord::SendToOther));
    typeWidget->addItem(tr("To yourself"), TransactionFilterProxy::TYPE(TransactionRecord::SendToSelf));
    typeWidget->addItem(tr("Mined"), TransactionFilterProxy::TYPE(TransactionRecord::Generated));
    typeWidget->addItem(tr("Other"), TransactionFilterProxy::TYPE(TransactionRecord::Other));
    typeWidget->setStyleSheet(themeManager->getCurrent()->getQComboboxTransactionsFilteringStyle());

    hlayout->addWidget(typeWidget);
    hlayout->insertSpacing(3, 8);

    frameForAddress = new QFrame();
    frameForAddress->setFixedHeight(44);
    frameForAddress->setFixedWidth(378);
    frameForAddress->setStyleSheet("background-color: #393947; padding-left: 5px");
    QHBoxLayout *hlayoutFrameForAddress = new QHBoxLayout();
    hlayoutFrameForAddress->setContentsMargins(0,0,0,0);
    frameForAddress->setLayout(hlayoutFrameForAddress);
    frameForAddress->setStyleSheet(themeManager->getCurrent()->getQFrameGeneralStyle());



    addressWidget = new QLineEdit(this);
    addressWidget->setStyleSheet(themeManager->getCurrent()->getQLabelGeneralStyle());

#if QT_VERSION >= 0x040700
    /* Do not move this to the XML file, Qt before 4.7 will choke on it */
    addressWidget->setPlaceholderText(tr("Enter address or label to search"));
#endif
    hlayoutFrameForAddress->addWidget(addressWidget);


    frameForAmount = new QFrame();
    frameForAmount->setFixedHeight(44);
    frameForAmount->setFixedWidth(117);
    frameForAmount->setStyleSheet("background-color: #393947; padding-left: 5px");
    QHBoxLayout *hlayoutFrameForAmount = new QHBoxLayout();
    hlayoutFrameForAmount->setContentsMargins(0,0,0,0);
    frameForAmount->setLayout(hlayoutFrameForAmount);
    frameForAmount->setStyleSheet(themeManager->getCurrent()->getQFrameGeneralStyle());

    amountWidget = new QLineEdit(this);
    amountWidget->setStyleSheet(themeManager->getCurrent()->getQLabelGeneralStyle());

#if QT_VERSION >= 0x040700
    /* Do not move this to the XML file, Qt before 4.7 will choke on it */
    amountWidget->setPlaceholderText(tr("Min amount"));
#endif
#ifdef Q_OS_MAC
    amountWidget->setFixedWidth(97);
#else
    amountWidget->setFixedWidth(100);
#endif
    amountWidget->setValidator(new QDoubleValidator(0, 1e20, 8, this));

    hlayoutFrameForAmount->addWidget(amountWidget);

    hlayout->addWidget(frameForAddress);
    hlayout->insertSpacing(5, 8);
    hlayout->addWidget(frameForAmount);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0,0,0,0);
    vlayout->setSpacing(0);

    QTableView *view = new QTableView(this);

    vlayout->addWidget(pageTitleLabel);
    vlayout->insertSpacing(1,40);

    QHBoxLayout *hlayout1 = new QHBoxLayout();
    hlayout1->setContentsMargins(0,0,0,0);
    hlayout1->setSpacing(0);
    hlayout1->insertSpacing(0, 60);
    QVBoxLayout *vlayout1 = new QVBoxLayout();
    vlayout1->setContentsMargins(0,0,0,0);
    vlayout1->setSpacing(0);
    vlayout1->addLayout(hlayout);
    vlayout1->addWidget(createDateRangeWidget());
    vlayout1->insertSpacing(2, 20);
    vlayout1->addWidget(view);

    hlayout1->addLayout(vlayout1);
    hlayout1->addSpacing(60);

    vlayout->addLayout(hlayout1);
    vlayout->addSpacing(20);
    vlayout->setSpacing(0);

    // Always show scroll bar
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    int width = view->verticalScrollBar()->sizeHint().width();
    // Cover scroll bar width with spacing
#ifdef Q_OS_MAC
    hlayout->addSpacing(width+2);
#else
    hlayout->addSpacing(width);
#endif

    view->setTabKeyNavigation(false);
    view->setContextMenuPolicy(Qt::CustomContextMenu);

    transactionView = view;

    // Actions
    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyLabelAction = new QAction(tr("Copy label"), this);
    QAction *copyAmountAction = new QAction(tr("Copy amount"), this);
    QAction *copyTxIDAction = new QAction(tr("Copy transaction ID"), this);
    QAction *editLabelAction = new QAction(tr("Edit label"), this);
    QAction *showDetailsAction = new QAction(tr("Show transaction details"), this);

    contextMenu = new QMenu();
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyLabelAction);
    contextMenu->addAction(copyAmountAction);
    contextMenu->addAction(copyTxIDAction);
    contextMenu->addAction(editLabelAction);
    contextMenu->addAction(showDetailsAction);

    // Connect actions
    connect(dateWidget, SIGNAL(activated(int)), this, SLOT(chooseDate(int)));
    connect(typeWidget, SIGNAL(activated(int)), this, SLOT(chooseType(int)));
    connect(addressWidget, SIGNAL(textChanged(QString)), this, SLOT(changedPrefix(QString)));
    connect(amountWidget, SIGNAL(textChanged(QString)), this, SLOT(changedAmount(QString)));

    connect(view, SIGNAL(doubleClicked(QModelIndex)), this, SIGNAL(doubleClicked(QModelIndex)));
    connect(view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyLabelAction, SIGNAL(triggered()), this, SLOT(copyLabel()));
    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));
    connect(copyTxIDAction, SIGNAL(triggered()), this, SLOT(copyTxID()));
    connect(editLabelAction, SIGNAL(triggered()), this, SLOT(editLabel()));
    connect(showDetailsAction, SIGNAL(triggered()), this, SLOT(showDetails()));
}

void TransactionView::setModel(WalletModel *model)
{
    this->model = model;
    if(model)
    {
        transactionProxyModel = new TransactionFilterProxy(this);
        transactionProxyModel->setSourceModel(model->getTransactionTableModel());
        transactionProxyModel->setDynamicSortFilter(true);
        transactionProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        transactionProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

        transactionProxyModel->setSortRole(Qt::EditRole);

        transactionView->setModel(transactionProxyModel);
        transactionView->setAlternatingRowColors(true);
        transactionView->setStyleSheet(themeManager->getCurrent()->getQTableGeneralStyle());
        transactionView->setSelectionBehavior(QAbstractItemView::SelectRows);
        transactionView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        transactionView->setSortingEnabled(true);
        transactionView->sortByColumn(TransactionTableModel::Date, Qt::DescendingOrder);
        transactionView->verticalHeader()->hide();

        transactionView->horizontalHeader()->resizeSection(
                TransactionTableModel::Status, 28);
        transactionView->horizontalHeader()->resizeSection(
                TransactionTableModel::Date, 120);
        transactionView->horizontalHeader()->resizeSection(
                TransactionTableModel::Type, 120);
        transactionView->horizontalHeader()->setSectionResizeMode(TransactionTableModel::ToAddress, QHeaderView::Stretch);
        //transactionView->horizontalHeader()->resizeSection(
        //        TransactionTableModel::Amount, 100);
        transactionView->horizontalHeader()->setStyleSheet(themeManager->getCurrent()->getQListHeaderGeneralStyle());
    }
}

void TransactionView::chooseDate(int idx)
{
    if(!transactionProxyModel)
        return;
    QDate current = QDate::currentDate();
    dateRangeWidget->setVisible(false);
    switch(dateWidget->itemData(idx).toInt())
    {
        case All:
            transactionProxyModel->setDateRange(
                    TransactionFilterProxy::MIN_DATE,
                    TransactionFilterProxy::MAX_DATE);
            break;
        case Today:
            transactionProxyModel->setDateRange(
                    QDateTime(current),
                    TransactionFilterProxy::MAX_DATE);
            break;
        case ThisWeek: {
            // Find last Monday
            QDate startOfWeek = current.addDays(-(current.dayOfWeek()-1));
            transactionProxyModel->setDateRange(
                    QDateTime(startOfWeek),
                    TransactionFilterProxy::MAX_DATE);

        } break;
        case ThisMonth:
            transactionProxyModel->setDateRange(
                    QDateTime(QDate(current.year(), current.month(), 1)),
                    TransactionFilterProxy::MAX_DATE);
            break;
        case LastMonth:
            transactionProxyModel->setDateRange(
                    QDateTime(QDate(current.year(), current.month()-1, 1)),
                    QDateTime(QDate(current.year(), current.month(), 1)));
            break;
        case ThisYear:
            transactionProxyModel->setDateRange(
                    QDateTime(QDate(current.year(), 1, 1)),
                    TransactionFilterProxy::MAX_DATE);
            break;
        case Range:
            dateRangeWidget->setVisible(true);
            dateRangeChanged();
            break;
    }
}

void TransactionView::chooseType(int idx)
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setTypeFilter(
            typeWidget->itemData(idx).toInt());
}

void TransactionView::changedPrefix(const QString &prefix)
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setAddressPrefix(prefix);
}

void TransactionView::changedAmount(const QString &amount)
{
    if(!transactionProxyModel)
        return;
    qint64 amount_parsed = 0;
    if(BitcoinUnits::parse(model->getOptionsModel()->getDisplayUnit(), amount, &amount_parsed))
    {
        transactionProxyModel->setMinAmount(amount_parsed);
    }
    else
    {
        transactionProxyModel->setMinAmount(0);
    }
}

void TransactionView::exportClicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(
            this,
            tr("Export Transaction Data"), QString(),
            tr("Comma separated file (*.csv)"));

    if (filename.isNull()) return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(transactionProxyModel);
    writer.addColumn(tr("Confirmed"), 0, TransactionTableModel::ConfirmedRole);
    writer.addColumn(tr("Date"), 0, TransactionTableModel::DateRole);
    writer.addColumn(tr("Type"), TransactionTableModel::Type, Qt::EditRole);
    writer.addColumn(tr("Label"), 0, TransactionTableModel::LabelRole);
    writer.addColumn(tr("Address"), 0, TransactionTableModel::AddressRole);
    writer.addColumn(tr("Amount"), 0, TransactionTableModel::FormattedAmountRole);
    writer.addColumn(tr("ID"), 0, TransactionTableModel::TxIDRole);

    if(!writer.write())
    {
        QMessageBox::critical(this, tr("Error exporting"), tr("Could not write to file %1.").arg(filename),
                              QMessageBox::Abort, QMessageBox::Abort);
    }
}

void TransactionView::contextualMenu(const QPoint &point)
{
    QModelIndex index = transactionView->indexAt(point);
    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void TransactionView::copyAddress()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::AddressRole);
}

void TransactionView::copyLabel()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::LabelRole);
}

void TransactionView::copyAmount()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::FormattedAmountRole);
}

void TransactionView::copyTxID()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::TxIDRole);
}

void TransactionView::editLabel()
{
    if(!transactionView->selectionModel() ||!model)
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        AddressTableModel *addressBook = model->getAddressTableModel();
        if(!addressBook)
            return;
        QString address = selection.at(0).data(TransactionTableModel::AddressRole).toString();
        if(address.isEmpty())
        {
            // If this transaction has no associated address, exit
            return;
        }
        // Is address in address book? Address book can miss address when a transaction is
        // sent from outside the UI.
        int idx = addressBook->lookupAddress(address);
        if(idx != -1)
        {
            // Edit sending / receiving address
            QModelIndex modelIdx = addressBook->index(idx, 0, QModelIndex());
            // Determine type of address, launch appropriate editor dialog type
            QString type = modelIdx.data(AddressTableModel::TypeRole).toString();

            EditAddressDialog dlg(type==AddressTableModel::Receive
                                  ? EditAddressDialog::EditReceivingAddress
                                  : EditAddressDialog::EditSendingAddress,
                                  this);
            dlg.setModel(addressBook);
            dlg.loadRow(idx);
            dlg.exec();
        }
        else
        {
            // Add sending address
            EditAddressDialog dlg(EditAddressDialog::NewSendingAddress,
                                  this);
            dlg.setModel(addressBook);
            dlg.setAddress(address);
            dlg.exec();
        }
    }
}

void TransactionView::showDetails()
{
    if(!transactionView->selectionModel())
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        TransactionDescDialog dlg(selection.at(0));
        dlg.exec();
    }
}

QWidget *TransactionView::createDateRangeWidget()
{
    dateRangeWidget = new QFrame();
    dateRangeWidget->setFrameStyle(QFrame::Panel);
    dateRangeWidget->setStyleSheet("border: none; padding-top: 5px; padding-bottom: 5px; padding-left: 6px;");
    QHBoxLayout *layout = new QHBoxLayout(dateRangeWidget);
    layout->setContentsMargins(0,0,0,0);
    layout->addSpacing(28);
    //layout->addWidget(new QLabel(tr("Range:")));

    dateFrom = new QDateTimeEdit(this);
    dateFrom->setStyleSheet(themeManager->getCurrent()->getQComboboxDateRangeStyle());
    dateFrom->setDisplayFormat("dd/MM/yy");
    dateFrom->setCalendarPopup(true);
    dateFrom->setMinimumWidth(100);
    dateFrom->setDate(QDate::currentDate().addDays(-7));
    layout->addWidget(dateFrom);
    layout->addWidget(new QLabel(tr("to")));

    dateTo = new QDateTimeEdit(this);
    dateTo->setStyleSheet(themeManager->getCurrent()->getQComboboxDateRangeStyle());
    dateTo->setDisplayFormat("dd/MM/yy");
    dateTo->setCalendarPopup(true);
    dateTo->setMinimumWidth(100);
    dateTo->setDate(QDate::currentDate());
    layout->addWidget(dateTo);
    layout->addStretch();

    // Hide by default
    dateRangeWidget->setVisible(false);

    // Notify on change
    connect(dateFrom, SIGNAL(dateChanged(QDate)), this, SLOT(dateRangeChanged()));
    connect(dateTo, SIGNAL(dateChanged(QDate)), this, SLOT(dateRangeChanged()));

    return dateRangeWidget;
}

void TransactionView::dateRangeChanged()
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setDateRange(
            QDateTime(dateFrom->date()),
            QDateTime(dateTo->date()).addDays(1));
}

void TransactionView::focusTransaction(const QModelIndex &idx)
{
    if(!transactionProxyModel)
        return;
    QModelIndex targetIdx = transactionProxyModel->mapFromSource(idx);
    transactionView->scrollTo(targetIdx);
    transactionView->setCurrentIndex(targetIdx);
    transactionView->setFocus();
}

void TransactionView::refreshStyle()
{
    pageTitleLabel->setStyleSheet(themeManager->getCurrent()->getMainHeaderStyle());
    transactionView->setStyleSheet(themeManager->getCurrent()->getQTableGeneralStyle());
    transactionView->horizontalHeader()->setStyleSheet(themeManager->getCurrent()->getQListHeaderGeneralStyle());
    dateWidget->setStyleSheet(themeManager->getCurrent()->getQComboboxDateRangeStyle());
    typeWidget->setStyleSheet(themeManager->getCurrent()->getQComboboxDateRangeStyle());
    frameForAmount->setStyleSheet(themeManager->getCurrent()->getQFrameGeneralStyle());
    frameForAddress->setStyleSheet(themeManager->getCurrent()->getQFrameGeneralStyle());
    amountWidget->setStyleSheet(themeManager->getCurrent()->getQLabelGeneralStyle());
    addressWidget->setStyleSheet(themeManager->getCurrent()->getQLabelGeneralStyle());
    dateFrom->setStyleSheet(themeManager->getCurrent()->getQComboboxDateRangeStyle());
    dateTo->setStyleSheet(themeManager->getCurrent()->getQComboboxDateRangeStyle());
}