/*
** -- Local Includes --
*/

#include "biblioteq.h"
#include "biblioteq_copy_editor_book.h"

extern biblioteq *qmain;

biblioteq_copy_editor_book::biblioteq_copy_editor_book
(QWidget *parent, biblioteq_item *bitemArg,
 const bool showForLendingArg,
 const int quantityArg, const QString &ioidArg,
 QSpinBox *spinboxArg,
 const QFont &font,
 const QString &uniqueIdArg):biblioteq_copy_editor(parent)
{
  if(parent == qmain->getMembersBrowser())
    setWindowModality(Qt::ApplicationModal);
  else
    setWindowModality(Qt::WindowModal);

  m_cb.setupUi(this);
  m_bitem = bitemArg;
  m_ioid = ioidArg;
  m_quantity = quantityArg;
  m_spinbox = spinboxArg;
  m_itemType = "Book";
  m_showForLending = showForLendingArg;
#if QT_VERSION >= 0x050000
  m_cb.table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
#else
  m_cb.table->verticalHeader()->setResizeMode(QHeaderView::Fixed);
#endif

  if(!uniqueIdArg.trimmed().isEmpty())
    setWindowTitle
      (tr("BiblioteQ: Copy Browser (%1)").arg(uniqueIdArg));
  else
    setWindowTitle(tr("BiblioteQ: Copy Browser"));

  setGlobalFonts(font);
}

biblioteq_copy_editor_book::~biblioteq_copy_editor_book()
{
  clearCopiesList();
}

void biblioteq_copy_editor_book::slotDeleteCopy(void)
{
  QString copyid = "";
  QString errorstr = "";
  bool isCheckedOut = false;
  int row = m_cb.table->currentRow();

  if(row < 0)
    {
      QMessageBox::critical(this, tr("BiblioteQ: User Error"),
			    tr("Please select the copy that you intend to "
			       "delete."));
      return;
    }
  else if(m_cb.table->rowCount() == 1)
    {
      QMessageBox::critical(this, tr("BiblioteQ: User Error"),
			    tr("You must have at least one copy."));
      return;
    }

  copyid = biblioteq_misc_functions::getColumnString
    (m_cb.table, row, m_columnHeaderIndexes.indexOf("Barcode"));
  QApplication::setOverrideCursor(Qt::WaitCursor);
  isCheckedOut = biblioteq_misc_functions::isCopyCheckedOut(qmain->getDB(),
							    copyid,
							    m_ioid,
							    m_itemType,
							    errorstr);
  QApplication::restoreOverrideCursor();

  if(isCheckedOut)
    {
      if(m_cb.table->item(row, 1) != 0)
	m_cb.table->item(row, 1)->setFlags(0);

      if(m_cb.table->item(row, 2) != 0)
	{
	  m_cb.table->item(row, 2)->setFlags(0);
	  m_cb.table->item(row, 2)->setText("0");
	}

      QMessageBox::critical(this, tr("BiblioteQ: User Error"),
			    tr("It appears that the copy you selected to "
			       "delete is reserved."));
      return;
    }
  else if(errorstr.length() > 0)
    {
      qmain->addError(QString(tr("Database Error")),
		      QString(tr("Unable to determine the reservation "
				 "status of the selected copy.")),
		      errorstr, __FILE__, __LINE__);
      QMessageBox::critical(this, tr("BiblioteQ: Database Error"),
			    tr("Unable to determine the reservation status "
			       "of the selected copy."));
      return;
    }

  m_cb.table->removeRow(m_cb.table->currentRow());
}

void biblioteq_copy_editor_book::populateCopiesEditor(void)
{
  QSqlQuery query(qmain->getDB());
  QString str = "";
  QStringList list;
  QTableWidgetItem *item = 0;
  bool terminate = false;
  int i = 0;
  int j = 0;
  int row = 0;

  m_cb.dueDateFrame->setVisible(m_showForLending);
  m_cb.deleteButton->setVisible(!m_showForLending);

  if(!m_showForLending)
    {
      m_cb.saveButton->setText(tr("&Save"));
      disconnect(m_cb.saveButton, SIGNAL(clicked(void)));
      disconnect(m_cb.deleteButton, SIGNAL(clicked(void)));
      connect(m_cb.saveButton, SIGNAL(clicked(void)), this,
	      SLOT(slotSaveCopies(void)));
      connect(m_cb.deleteButton, SIGNAL(clicked(void)), this,
	      SLOT(slotDeleteCopy(void)));
    }
  else
    {
      /*
      ** Set the minimum date to duedate.
      */

      QDate duedate = QDate::currentDate();
      QString errorstr("");

      QApplication::setOverrideCursor(Qt::WaitCursor);
      duedate = duedate.addDays
	(biblioteq_misc_functions::getMinimumDays
	 (qmain->getDB(),
	  m_itemType,
	  errorstr));
      QApplication::restoreOverrideCursor();

      if(!errorstr.isEmpty())
	qmain->addError(QString(tr("Database Error")),
			QString(tr("Unable to retrieve "
				   "the minimum number of days.")),
			errorstr, __FILE__, __LINE__);

      m_cb.dueDate->setMinimumDate(duedate);
      m_cb.saveButton->setText(tr("&Reserve"));
      disconnect(m_cb.saveButton, SIGNAL(clicked(void)));
      connect(m_cb.saveButton, SIGNAL(clicked(void)), this,
	      SLOT(slotCheckoutCopy(void)));
    }

  disconnect(m_cb.cancelButton, SIGNAL(clicked(void)));
  connect(m_cb.cancelButton, SIGNAL(clicked(void)), this,
	  SLOT(slotCloseCopyEditor(void)));

  QProgressDialog progress1(this);
  QProgressDialog progress2(this);

  m_cb.table->setColumnCount(0);
  m_cb.table->setRowCount(0);
  list.append(tr("Title"));
  list.append(tr("Barcode"));
  list.append(tr("Availability"));
  list.append(tr("Originality"));
  list.append(tr("Condition"));
  list.append("ITEM_OID");
  list.append("Copy Number");
  m_columnHeaderIndexes.clear();
  m_columnHeaderIndexes.append("Title");
  m_columnHeaderIndexes.append("Barcode");
  m_columnHeaderIndexes.append("Availability");
  m_columnHeaderIndexes.append("Originality");
  m_columnHeaderIndexes.append("Condition");
  m_columnHeaderIndexes.append("ITEM_OID");
  m_columnHeaderIndexes.append("Copy Number");
  m_cb.table->setColumnCount(list.size());
  m_cb.table->setHorizontalHeaderLabels(list);
  list.clear();
  m_cb.table->setRowCount(m_quantity);
  m_cb.table->scrollToTop();
  m_cb.table->horizontalScrollBar()->setValue(0);

  /*
  ** Hide the Copy Number and ITEM_OID columns.
  */

  m_cb.table->setColumnHidden(m_cb.table->columnCount() - 1, true);
  m_cb.table->setColumnHidden(m_cb.table->columnCount() - 2, true);
  updateGeometry();
  show();
  progress1.setModal(true);
  progress1.setWindowTitle(tr("BiblioteQ: Progress Dialog"));
  progress1.setLabelText(tr("Constructing objects..."));
  progress1.setMaximum(m_quantity);
  progress1.setMinimum(0);
  progress1.show();
  progress1.repaint();
#ifndef Q_OS_MAC
  QApplication::processEvents();
#endif

  for(i = 0; i < m_quantity && !progress1.wasCanceled(); i++)
    {
      for(j = 0; j < m_cb.table->columnCount(); j++)
	if(j == 3 || j == 4)
	  {
	    QComboBox *combobox = new(std::nothrow) QComboBox();

	    if(combobox)
	      {
		QStringList list;

		if(j == 3)
		  list << tr("Black & White Copy")
		       << tr("Color Copy")
		       << tr("Original");
		else
		  list << tr("As New")
		       << tr("Binding Copy")
		       << tr("Book Club")
		       << tr("Ex-Library")
		       << tr("Fair")
		       << tr("Fine")
		       << tr("Good")
		       << tr("Poor")
		       << tr("Very Good");

		combobox->addItems(list);

		if(j == 3)
		  combobox->setCurrentIndex(2);

		m_cb.table->setCellWidget(i, j, combobox);
	      }
	    else
	      qmain->addError(QString(tr("Memory Error")),
			      QString(tr("Unable to allocate memory for the "
					 "\"combobox\" object. "
					 "This is a serious problem!")),
			      QString(""), __FILE__, __LINE__);
	  }
	else if((item = new(std::nothrow) QTableWidgetItem()) != 0)
	  {
	    if(m_showForLending)
	      item->setFlags(0);
	    else if(j == 1)
	      item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled |
			     Qt::ItemIsEditable);
	    else
	      item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

	    if(j == 1)
	      item->setText("");
	    else if(j == 2)
	      {
		if(m_showForLending)
		  item->setText("0");
		else
		  item->setText("1");
	      }
	    else if(j == 5)
	      item->setText(m_ioid);
	    else
	      item->setText("");

	    m_cb.table->setItem(i, j, item);
	  }
	else
	  qmain->addError(QString(tr("Memory Error")),
			  QString(tr("Unable to allocate memory for the "
				     "\"item\" object. "
				     "This is a serious problem!")),
			  QString(""), __FILE__, __LINE__);

      if(i + 1 <= progress1.maximum())
	progress1.setValue(i + 1);

      progress1.repaint();
#ifndef Q_OS_MAC
      QApplication::processEvents();
#endif
    }

  progress1.close();
  m_cb.table->setRowCount(i); // Support cancellation.
  query.prepare(QString("SELECT %1.title, "
			"%1_copy_info.copyid, "
			"(1 - COUNT(item_borrower_vw.copyid)), "
			"%1_copy_info.originality, "
			"%1_copy_info.condition, "
			"%1_copy_info.item_oid, "
			"%1_copy_info.copy_number "
			"FROM "
			"%1, "
			"%1_copy_info LEFT JOIN item_borrower_vw ON "
			"%1_copy_info.copyid = "
			"item_borrower_vw.copyid AND "
			"%1_copy_info.item_oid = "
			"item_borrower_vw.item_oid AND "
			"item_borrower_vw.type = ? "
			"WHERE %1_copy_info.item_oid = ? AND "
			"%1.myoid = ? "
			"GROUP BY %1.title, "
			"%1_copy_info.copyid, "
			"%1_copy_info.originality, "
			"%1_copy_info.condition, "
			"%1_copy_info.item_oid, "
			"%1_copy_info.copy_number "
			"ORDER BY %1_copy_info.copy_number").arg
		(m_itemType.toLower().remove(" ")));
  query.bindValue(0, m_itemType);
  query.bindValue(1, m_ioid);
  query.bindValue(2, m_ioid);
  QApplication::setOverrideCursor(Qt::WaitCursor);

  if(!query.exec())
    qmain->addError(QString(tr("Database Error")),
		    QString(tr("Unable to retrieve copy data.")),
		    query.lastError().text(), __FILE__, __LINE__);

  QApplication::restoreOverrideCursor();
  progress2.setModal(true);
  progress2.setWindowTitle(tr("BiblioteQ: Progress Dialog"));
  progress2.setLabelText(tr("Retrieving copy information..."));
  progress2.setMinimum(0);

  /*
  ** SQLite does not support query.size().
  */

  if(qmain->getDB().driverName() == "QSQLITE")
    {
      if(query.lastError().isValid())
	progress2.setMaximum(0);
      else
	progress2.setMaximum
	  (biblioteq_misc_functions::sqliteQuerySize(query.lastQuery(),
						     query.boundValues(),
						     qmain->getDB(),
						     __FILE__, __LINE__));
    }
  else
    progress2.setMaximum(query.size());

  progress2.show();
  progress2.repaint();
#ifndef Q_OS_MAC
  QApplication::processEvents();
#endif
  i = -1;

  while(i++, !progress2.wasCanceled() && query.next())
    {
      if(query.isValid())
	{
	  row = query.value(6).toInt() - 1;

	  for(j = 0; j < m_cb.table->columnCount(); j++)
	    if(m_cb.table->item(row, j) != 0)
	      {
		str = query.value(j).toString();

		if(query.value(2).toString() == "0")
		  m_cb.table->item(row, j)->setFlags(0);
		else if(m_showForLending)
		  {
		    m_cb.table->item(row, j)->setFlags
		      (Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		    if(m_cb.table->currentRow() == -1)
		      m_cb.table->selectRow(row);
		  }
		else if(j == 1)
		  m_cb.table->item(row, 1)->setFlags
		    (Qt::ItemIsSelectable | Qt::ItemIsEnabled |
		     Qt::ItemIsEditable);

		if(j == 2)
		  m_cb.table->item(row, 2)->setText(query.value(2).toString());
		else
		  {
		    if(i == 0 && j == 0)
		      m_cb.table->item(row, j)->setText(str);
		    else if(j != 0)
		      m_cb.table->item(row, j)->setText(str);
		  }
	      }
	    else if(m_cb.table->cellWidget(row, j) != 0)
	      {
		QComboBox *combobox = qobject_cast<QComboBox *>
		  (m_cb.table->cellWidget(row, j));

		if(combobox)
		  {
		    str = query.value(j).toString();

		    if(combobox->findText(str) > -1)
		      combobox->setCurrentIndex(combobox->findText(str));
		  }
	      }
	    else
	      terminate = true;
	}

      if(i + 1 <= progress2.maximum())
	progress2.setValue(i + 1);

      progress2.repaint();
#ifndef Q_OS_MAC
      QApplication::processEvents();
#endif

      if(terminate)
	break; // Out of resources?
    }

  progress2.close();

  for(int i = 0; i < m_cb.table->rowCount() - 1; i++)
    m_cb.table->resizeColumnToContents(i);
}

void biblioteq_copy_editor_book::slotSaveCopies(void)
{
  QComboBox *combobox1 = 0;
  QComboBox *combobox2 = 0;
  QString availability = "";
  QString errormsg = "";
  QString errorstr = "";
  QString str = "";
  QStringList duplicates;
  QTableWidgetItem *item1 = 0;
  QTableWidgetItem *item2 = 0;
  copy_class *copy = 0;
  int i = 0;

  m_cb.table->setFocus();

  for(i = 0; i < m_cb.table->rowCount(); i++)
    if(m_cb.table->item(i, 1) != 0 &&
       m_cb.table->item(i, 1)->text().trimmed().isEmpty())
      {
	errormsg = QString(tr("Row number ")) + QString::number(i + 1) +
	  tr(" contains an empty Barcode.");
	QMessageBox::critical(this, tr("BiblioteQ: User Error"), errormsg);
	duplicates.clear();
	return;
      }
    else if(m_cb.table->item(i, 1) != 0)
      {
	if(duplicates.contains(m_cb.table->item(i, 1)->text()))
	  {
	    errormsg = QString(tr("Row number ")) + QString::number(i + 1) +
	      tr(" contains a duplicate Barcode.");
	    QMessageBox::critical(this, tr("BiblioteQ: User Error"),
				  errormsg);
	    duplicates.clear();
	    return;
	  }
	else
	  duplicates.append(m_cb.table->item(i, 1)->text());
      }

  duplicates.clear();

  while(!m_copies.isEmpty())
    delete m_copies.takeFirst();

  QApplication::setOverrideCursor(Qt::WaitCursor);

  if(!qmain->getDB().transaction())
    {
      QApplication::restoreOverrideCursor();
      qmain->addError(QString(tr("Database Error")),
		      QString(tr("Unable to create a database transaction.")),
		      qmain->getDB().lastError().text(), __FILE__, __LINE__);
      QMessageBox::critical(this, tr("BiblioteQ: Database Error"),
			    tr("Unable to create a database transaction."));
      return;
    }

  QApplication::restoreOverrideCursor();

  for(i = 0; i < m_cb.table->rowCount(); i++)
    {
      combobox1 = qobject_cast<QComboBox *> (m_cb.table->cellWidget(i, 3));
      combobox2 = qobject_cast<QComboBox *> (m_cb.table->cellWidget(i, 4));
      item1 = m_cb.table->item(i, 1);
      item2 = m_cb.table->item(i, 5);

      if(combobox1 == 0 || combobox2 == 0 || item1 == 0 || item2 == 0)
	continue;

      if((copy = new(std::nothrow) copy_class
	  (combobox2->currentText().trimmed(),
	   item1->text().trimmed(),
	   item2->text(),
	   combobox1->currentText().trimmed())) != 0)
	m_copies.append(copy);
      else
	qmain->addError
	  (QString(tr("Memory Error")),
	   QString(tr("Unable to allocate memory for the \"copy\" object. "
		      "This is a serious problem!")),
	   QString(""), __FILE__, __LINE__);
    }

  if(saveCopies().isEmpty())
    {
      QApplication::setOverrideCursor(Qt::WaitCursor);
      biblioteq_misc_functions::saveQuantity
	(qmain->getDB(), m_ioid, m_copies.size(),
	 m_itemType, errorstr);
      QApplication::restoreOverrideCursor();

      if(!errorstr.isEmpty())
	qmain->addError(QString(tr("Database Error")),
			QString(tr("Unable to save the item's quantity.")),
			errorstr, __FILE__, __LINE__);
      else
	goto success_label;
    }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  while(!m_copies.isEmpty())
    delete m_copies.takeFirst();

  if(!qmain->getDB().rollback())
    qmain->addError(QString(tr("Database Error")),
		    QString(tr("Rollback failure.")),
		    qmain->getDB().lastError().text(), __FILE__, __LINE__);

  QApplication::restoreOverrideCursor();
  QMessageBox::critical(this, tr("BiblioteQ: Database Error"),
			tr("Unable to save the copy data."));
  return;

 success_label:

  QApplication::setOverrideCursor(Qt::WaitCursor);

  if(!qmain->getDB().commit())
    {
      while(!m_copies.isEmpty())
	delete m_copies.takeFirst();

      qmain->addError(QString(tr("Database Error")),
		      QString(tr("Commit failure.")),
		      qmain->getDB().lastError().text(), __FILE__, __LINE__);
      qmain->getDB().rollback();
      QApplication::restoreOverrideCursor();
      QMessageBox::critical(this, tr("BiblioteQ: Database Error"),
			    tr("Unable to commit the copy data."));
      return;
    }

  QApplication::restoreOverrideCursor();

  if(m_spinbox)
    m_spinbox->setValue(m_copies.size());

  /*
    QApplication::setOverrideCursor(Qt::WaitCursor);
    availability = biblioteq_misc_functions::getAvailability
    (m_ioid, qmain->getDB(),
    m_itemType, errorstr);
    QApplication::restoreOverrideCursor();

    if(!errorstr.isEmpty())
    qmain->addError(QString(tr("Database Error")),
    QString(tr("Retrieving availability.")),
    errorstr, __FILE__, __LINE__);

    biblioteq_misc_functions::updateColumn
    (qmain->getUI().table, m_bitem->getRow(),
    tr("Availability"), availability);
    str.setNum(m_copies.size());
    biblioteq_misc_functions::updateColumn
    (qmain->getUI().table, m_bitem->getRow(),
    tr("Quantity"), str);
  */

  if(m_bitem)
    m_bitem->setOldQ(m_copies.size());

  while(!m_copies.isEmpty())
    delete m_copies.takeFirst();
}

void biblioteq_copy_editor_book::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    slotCloseCopyEditor();

  QDialog::keyPressEvent(event);
}

void biblioteq_copy_editor_book::closeEvent(QCloseEvent *event)
{
  Q_UNUSED(event);
  slotCloseCopyEditor();
}

void biblioteq_copy_editor_book::changeEvent(QEvent *event)
{
  if(event)
    switch(event->type())
      {
      case QEvent::LanguageChange:
	{
	  m_cb.retranslateUi(this);
	  break;
	}
      default:
	break;
      }

  QDialog::changeEvent(event);
}

QString biblioteq_copy_editor_book::saveCopies(void)
{
  QProgressDialog progress(this);
  QSqlQuery query(qmain->getDB());
  QString lastError = "";
  copy_class *copy = 0;
  int i = 0;

  query.prepare(QString("DELETE FROM %1_copy_info WHERE "
			"item_oid = ?").arg(m_itemType.toLower().remove(" ")));
  query.bindValue(0, m_ioid);
  QApplication::setOverrideCursor(Qt::WaitCursor);

  if(!query.exec())
    {
      QApplication::restoreOverrideCursor();
      qmain->addError(QString(tr("Database Error")),
		      QString(tr("Unable to purge copy data.")),
		      query.lastError().text(), __FILE__, __LINE__);
      return query.lastError().text();
    }
  else
    {
      QApplication::restoreOverrideCursor();
      progress.setCancelButton(0);
      progress.setModal(true);
      progress.setWindowTitle(tr("BiblioteQ: Progress Dialog"));
      progress.setLabelText(tr("Saving the copy data..."));
      progress.setMaximum(m_copies.size());
      progress.setMinimum(0);
      progress.show();
      progress.repaint();
#ifndef Q_OS_MAC
      QApplication::processEvents();
#endif

      for(i = 0; i < m_copies.size(); i++)
	{
	  copy = m_copies.at(i);

	  if(!copy)
	    goto continue_label;

	  if(qmain->getDB().driverName() != "QSQLITE")
	    query.prepare(QString("INSERT INTO %1_copy_info "
				  "(item_oid, copy_number, "
				  "copyid, originality, condition) "
				  "VALUES (?, ?, ?, "
				  "?, ?)").arg
			  (m_itemType.toLower().remove(" ")));
	  else
	    query.prepare(QString("INSERT INTO %1_copy_info "
				  "(item_oid, copy_number, "
				  "copyid, originality, condition, myoid) "
				  "VALUES (?, ?, ?, "
				  "?, ?, ?)").arg
			  (m_itemType.toLower().remove(" ")));

	  query.bindValue(0, copy->m_itemoid);
	  query.bindValue(1, i + 1);
	  query.bindValue(2, copy->m_copyid);
	  query.bindValue(3, copy->m_originality);
	  query.bindValue(4, copy->m_condition);

	  if(qmain->getDB().driverName() == "QSQLITE")
	    {
	      qint64 value = 0;
	      QString errorstr("");

	      value = biblioteq_misc_functions::getSqliteUniqueId
		(qmain->getDB(),
		 errorstr);

	      if(errorstr.isEmpty())
		query.bindValue(5, value);
	      else
		qmain->addError(QString(tr("Database Error")),
				QString(tr("Unable to generate a unique "
					   "integer.")),
				errorstr);
	    }

	  if(!query.exec())
	    {
	      lastError = query.lastError().text();
	      qmain->addError(QString(tr("Database Error")),
			      QString(tr("Unable to create copy data.")),
			      query.lastError().text(), __FILE__, __LINE__);
	    }

	continue_label:

	  if(i + 1 <= progress.maximum())
	    progress.setValue(i + 1);

	  progress.repaint();
#ifndef Q_OS_MAC
	  QApplication::processEvents();
#endif
	}

      progress.close();
    }

  return lastError;
}
