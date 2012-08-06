
/*LICENSE_START*/
/*
 * Copyright 2012 Washington University,
 * All rights reserved.
 *
 * Connectome DB and Connectome Workbench are part of the integrated Connectome 
 * Informatics Platform.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the names of Washington University nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR  
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*LICENSE_END*/

#define __CLASS_AND_NAME_HIERARCHY_VIEW_CONTROLLER_DECLARE__
#include "ClassAndNameHierarchyViewController.h"
#undef __CLASS_AND_NAME_HIERARCHY_VIEW_CONTROLLER_DECLARE__

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include "Brain.h"
#include "BorderFile.h"
#include "BrowserTabContent.h"
#include "CaretAssert.h"
#include "ClassAndNameHierarchyModel.h"
#include "ClassAndNameHierarchySelectedItem.h"
//#include "DisplayPropertiesBorders.h"
#include "FociFile.h"
#include "GuiManager.h"
#include "GiftiLabel.h"
#include "GiftiLabelTable.h"
#include "LabelFile.h"
#include "WuQtUtilities.h"
#include "WuQTreeWidget.h"

using namespace caret;


    
/**
 * \class caret::ClassAndNameHierarchyViewController 
 * \brief View controller for ClassAndNameHierarchyModels
 *
 * A view controller for one or more ClassAndNameHierarchyModel
 * instances.
 */

/**
 * Constructor.
 * @param parent
 *    Parent widget.
 */
ClassAndNameHierarchyViewController::ClassAndNameHierarchyViewController(const int32_t browserWindowIndex,
                                                                         QWidget* parent)
: QWidget(parent)
{
    m_alwaysDisplayNames = false;
    
    m_displayGroup = DisplayGroupEnum::getDefaultValue();
    m_browserWindowIndex = browserWindowIndex;
    m_treeWidget = new WuQTreeWidget();
    m_treeWidget->setStyleSheet("background-color: rgba(125,125,125,0)");
    m_treeWidget->setColumnCount(1);
    QObject::connect(m_treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
                     this, SLOT(treeWidgetItemChanged(QTreeWidgetItem*,int)));
    QObject::connect(m_treeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
                     this, SLOT(treeWidgetItemCollapsed(QTreeWidgetItem*)));
    QObject::connect(m_treeWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)),
                     this, SLOT(treeWidgetItemExpanded(QTreeWidgetItem*)));
    
    QWidget* allOnOffWidget = createAllOnOffControls();
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    WuQtUtilities::setLayoutMargins(layout, 0, 0);
    layout->addWidget(allOnOffWidget);
    layout->addSpacing(5);
    layout->addWidget(m_treeWidget);
    
}

/**
 * Destructor.
 */
ClassAndNameHierarchyViewController::~ClassAndNameHierarchyViewController()
{
    deleteItemSelectionInfo();
}

/**
 * Create buttons for all on and off
 */
QWidget* 
ClassAndNameHierarchyViewController::createAllOnOffControls()
{
    QLabel* allLabel = new QLabel("All: ");
    
    QPushButton* onPushButton = new QPushButton("On");
    QObject::connect(onPushButton, SIGNAL(clicked()),
                     this, SLOT(allOnPushButtonClicked()));
    
    QPushButton* offPushButton = new QPushButton("Off");
    QObject::connect(offPushButton, SIGNAL(clicked()),
                     this, SLOT(allOffPushButtonClicked()));
    
    QWidget* w = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(w);
    layout->addWidget(allLabel);
    layout->addWidget(onPushButton);
    layout->addWidget(offPushButton);
    layout->addStretch();
    return w;
}

/**
 * Called when all on push button clicked.
 */
void 
ClassAndNameHierarchyViewController::allOnPushButtonClicked()
{
    setAllSelected(true);
}

/**
 * Called when all off push button clicked.
 */
void 
ClassAndNameHierarchyViewController::allOffPushButtonClicked()
{
    setAllSelected(false);
}

/**
 * Set selection status of all items.
 * @param selected
 *    New selection status for all items.
 */
void 
ClassAndNameHierarchyViewController::setAllSelected(bool selected)
{
    BrowserTabContent* browserTabContent = 
    GuiManager::get()->getBrowserTabContentForBrowserWindow(m_browserWindowIndex, false);
    if (browserTabContent != NULL) {
        const int32_t browserTabIndex = browserTabContent->getTabNumber();
        for (std::vector<ClassAndNameHierarchyModel*>::iterator iter = m_classAndNameHierarchyModels.begin();
             iter != m_classAndNameHierarchyModels.end();
             iter++) {
            ClassAndNameHierarchyModel* model = *iter;
            model->setAllSelected(m_displayGroup,
                                  browserTabIndex,
                                  selected);
        }
        updateContents(m_classAndNameHierarchyModels);
    }
}


/**
 * Called when an item in the border selection tree widget
 * is changed.
 * @param item
 *    Item that is changed.
 * @param column
 *    Not used.
 */
void 
ClassAndNameHierarchyViewController::treeWidgetItemChanged(QTreeWidgetItem* item,
                                                          int /*column*/)
{
    void* ptr = item->data(0, Qt::UserRole).value<void*>();
    ClassAndNameHierarchySelectedItem* selectionInfo = (ClassAndNameHierarchySelectedItem*)ptr;
    CaretAssert(selectionInfo);
    
    const bool isSelected = (item->checkState(0) == Qt::Checked);
    
    BrowserTabContent* browserTabContent = 
       GuiManager::get()->getBrowserTabContentForBrowserWindow(m_browserWindowIndex, false);
    const int32_t browserTabIndex = browserTabContent->getTabNumber();
//    DisplayPropertiesBorders* displayPropertiesBorders = GuiManager::get()->getBrain()->getDisplayPropertiesBorders();
//    const DisplayGroupEnum::Enum displayGroup = displayPropertiesBorders->getDisplayGroupForTab(browserTabIndex);
    
    switch (selectionInfo->getItemType()) {
        case ClassAndNameHierarchySelectedItem::ITEM_TYPE_HIERARCHY_MODEL:
        {
            ClassAndNameHierarchyModel* hierarchyModel = selectionInfo->getClassAndNameHierarchyModel();
            CaretAssert(hierarchyModel);
            hierarchyModel->setSelected(m_displayGroup,
                                        browserTabIndex,
                                        isSelected);
        }
            break;
        case ClassAndNameHierarchySelectedItem::ITEM_TYPE_CLASS:
        {
            ClassAndNameHierarchyModel::ClassDisplayGroupSelector* classSelector = selectionInfo->getClassDisplayGroupSelector();
            classSelector->setSelected(m_displayGroup,
                                       browserTabIndex,
                                       isSelected);
        }
            break;
        case ClassAndNameHierarchySelectedItem::ITEM_TYPE_NAME:
        {
            ClassAndNameHierarchyModel::NameDisplayGroupSelector* nameSelector = selectionInfo->getNameDisplayGroupSelector();
            nameSelector->setSelected(m_displayGroup,
                                      browserTabIndex,
                                      isSelected);
        }
            break;
    }
    
    emit itemSelected(selectionInfo);
}

/**
 * Called when an item in the border selection tree widget
 * is collapsed.
 * @param item
 *    Item that is collapsed.
 */
void 
ClassAndNameHierarchyViewController::treeWidgetItemCollapsed(QTreeWidgetItem* item)
{    
    void* ptr = item->data(0, Qt::UserRole).value<void*>();
    ClassAndNameHierarchySelectedItem* selectionInfo = (ClassAndNameHierarchySelectedItem*)ptr;
    CaretAssert(selectionInfo);
    
    BrowserTabContent* browserTabContent = 
    GuiManager::get()->getBrowserTabContentForBrowserWindow(m_browserWindowIndex, false);
    const int32_t browserTabIndex = browserTabContent->getTabNumber();
//    DisplayPropertiesBorders* displayPropertiesBorders = GuiManager::get()->getBrain()->getDisplayPropertiesBorders();
//    DisplayGroupEnum::Enum displayGroup = displayPropertiesBorders->getDisplayGroupForTab(browserTabIndex);
    
    switch (selectionInfo->getItemType()) {
        case ClassAndNameHierarchySelectedItem::ITEM_TYPE_HIERARCHY_MODEL:
        {
            ClassAndNameHierarchyModel* hierarchyModel = selectionInfo->getClassAndNameHierarchyModel();
            CaretAssert(hierarchyModel);
            hierarchyModel->setExpanded(m_displayGroup,
                                        browserTabIndex,
                                        false);
        }
            break;
        case ClassAndNameHierarchySelectedItem::ITEM_TYPE_CLASS:
        {
            ClassAndNameHierarchyModel::ClassDisplayGroupSelector* classSelector = selectionInfo->getClassDisplayGroupSelector();
            CaretAssert(classSelector);
            classSelector->setExpanded(m_displayGroup,
                                       browserTabIndex,
                                       false);
        }
            break;
        case ClassAndNameHierarchySelectedItem::ITEM_TYPE_NAME:
            break;
    }
}

/**
 * Called when an item in the border selection tree widget
 * is expanded.
 * @param item
 *    Item that is expanded.
 */
void 
ClassAndNameHierarchyViewController::treeWidgetItemExpanded(QTreeWidgetItem* item)
{
    void* ptr = item->data(0, Qt::UserRole).value<void*>();
    ClassAndNameHierarchySelectedItem* selectionInfo = (ClassAndNameHierarchySelectedItem*)ptr;
    CaretAssert(selectionInfo);
    
    BrowserTabContent* browserTabContent = 
    GuiManager::get()->getBrowserTabContentForBrowserWindow(m_browserWindowIndex, false);
    const int32_t browserTabIndex = browserTabContent->getTabNumber();
//    DisplayPropertiesBorders* displayPropertiesBorders = GuiManager::get()->getBrain()->getDisplayPropertiesBorders();
//    DisplayGroupEnum::Enum displayGroup = displayPropertiesBorders->getDisplayGroupForTab(browserTabIndex);
    
    switch (selectionInfo->getItemType()) {
        case ClassAndNameHierarchySelectedItem::ITEM_TYPE_HIERARCHY_MODEL:
        {
            ClassAndNameHierarchyModel* hierarchyModel = selectionInfo->getClassAndNameHierarchyModel();
            CaretAssert(hierarchyModel);
            hierarchyModel->setExpanded(m_displayGroup,
                                        browserTabIndex,
                                        true);
        }
            break;
        case ClassAndNameHierarchySelectedItem::ITEM_TYPE_CLASS:
        {
            ClassAndNameHierarchyModel::ClassDisplayGroupSelector* classSelector = selectionInfo->getClassDisplayGroupSelector();
            CaretAssert(classSelector);
            classSelector->setExpanded(m_displayGroup, 
                                       browserTabIndex,
                                       true);
        }
            break;
        case ClassAndNameHierarchySelectedItem::ITEM_TYPE_NAME:
            break;
    }
}

/**
 * Update with border files.
 * @param borderFiles
 *    The border files.
 * @param displayGroup
 *    The selected display group.
 */
void 
ClassAndNameHierarchyViewController::updateContents(std::vector<BorderFile*> borderFiles,
                                                    const DisplayGroupEnum::Enum displayGroup)
{
    m_displayGroup = displayGroup;
    std::vector<ClassAndNameHierarchyModel*> classAndNameHierarchyModels;
    for (std::vector<BorderFile*>::iterator iter = borderFiles.begin();
         iter != borderFiles.end();
         iter++) {
        BorderFile* bf = *iter;
        CaretAssert(bf);
        classAndNameHierarchyModels.push_back(bf->getClassAndNameHierarchyModel());
    }
    
    updateContents(classAndNameHierarchyModels);
    
    m_treeWidget->resizeToFitContent();
}

/**
 * Update with border files.
 * @param borderFiles
 *    The border files.
 * @param displayGroup
 *    The selected display group.
 */
void 
ClassAndNameHierarchyViewController::updateContents(std::vector<FociFile*> fociFiles,
                                                    const DisplayGroupEnum::Enum displayGroup)
{
    m_displayGroup = displayGroup;
    std::vector<ClassAndNameHierarchyModel*> classAndNameHierarchyModels;
    for (std::vector<FociFile*>::iterator iter = fociFiles.begin();
         iter != fociFiles.end();
         iter++) {
        FociFile* ff = *iter;
        CaretAssert(ff);
        classAndNameHierarchyModels.push_back(ff->getClassAndNameHierarchyModel());
    }
    
    updateContents(classAndNameHierarchyModels);
    
    m_treeWidget->resizeToFitContent();
}

/**
 * Update with label files.
 * @param labelFiles
 *    The label files.
 * @param displayGroup
 *    The selected display group.
 */
void
ClassAndNameHierarchyViewController::updateContents(std::vector<LabelFile*> labelFiles,
                                                    const DisplayGroupEnum::Enum displayGroup)
{
    m_alwaysDisplayNames = true;
    m_displayGroup = displayGroup;
    std::vector<ClassAndNameHierarchyModel*> classAndNameHierarchyModels;
    for (std::vector<LabelFile*>::iterator iter = labelFiles.begin();
         iter != labelFiles.end();
         iter++) {
        LabelFile* lf = *iter;
        CaretAssert(lf);
        classAndNameHierarchyModels.push_back(lf->getClassAndNameHierarchyModel());
    }
    
    updateContents(classAndNameHierarchyModels);
    
    m_treeWidget->resizeToFitContent();
}

void
ClassAndNameHierarchyViewController::deleteItemSelectionInfo()
{
    for (std::vector<ClassAndNameHierarchySelectedItem*>::iterator itemSelIter = m_itemSelectionInfo.begin();
         itemSelIter != m_itemSelectionInfo.end();
         itemSelIter++) {
        delete *itemSelIter;
    }
    m_itemSelectionInfo.clear();
}


/**
 * Update the content of the view controller.
 * @param classAndNameHierarchyModels
 *    ClassAndNameHierarchyModels instances for display.
 * @param allowNamesWithZeroCounts
 *    If true, display names even if the usage count is zero.
 */
void 
ClassAndNameHierarchyViewController::updateContents(std::vector<ClassAndNameHierarchyModel*>& classAndNameHierarchyModels)
{
    /*
     * Copy the models
     */
    m_classAndNameHierarchyModels = classAndNameHierarchyModels;
    
    /*
     * Remove everything from the tree widget
     */
    m_treeWidget->blockSignals(true);
    m_treeWidget->clear();
    deleteItemSelectionInfo();
    
    BrowserTabContent* browserTabContent = 
        GuiManager::get()->getBrowserTabContentForBrowserWindow(m_browserWindowIndex, false);
    const int32_t browserTabIndex = browserTabContent->getTabNumber();
//    DisplayPropertiesBorders* displayPropertiesBorders = GuiManager::get()->getBrain()->getDisplayPropertiesBorders();
//    DisplayGroupEnum::Enum displayGroup = displayPropertiesBorders->getDisplayGroupForTab(browserTabIndex);
    
    /*
     * Loop through the models.
     */
    const int32_t numberOfModels = static_cast<int32_t>(m_classAndNameHierarchyModels.size());
    for (int32_t iModel = 0; iModel < numberOfModels; iModel++) {
        ClassAndNameHierarchyModel* classNamesModel = m_classAndNameHierarchyModels[iModel];
        
        QList<QTreeWidgetItem*> classTreeWidgets;
        
        /*
         * Loop through each class
         */
        const std::vector<int32_t> classKeysVector = classNamesModel->getAllClassKeysSortedByName();
        for (std::vector<int32_t>::const_iterator classIter = classKeysVector.begin();
             classIter != classKeysVector.end();
             classIter++) {
            const int32_t classKey = *classIter;
            ClassAndNameHierarchyModel::ClassDisplayGroupSelector* classSelector = classNamesModel->getClassSelectorForClassKey(classKey);
            CaretAssert(classSelector);
                
            /*
             * Names in the model
             */
            QList<QTreeWidgetItem*> nameTreeWidgets;
            const std::vector<int32_t> nameKeysVector = classSelector->getAllNameKeysSortedByName();
            for (std::vector<int32_t>::const_iterator nameIter = nameKeysVector.begin();
                 nameIter != nameKeysVector.end();
                 nameIter++) {
                const int32_t nameKey = *nameIter;
                ClassAndNameHierarchyModel::NameDisplayGroupSelector* nameSelector = classSelector->getNameSelectorWithKey(nameKey);
                CaretAssert(nameSelector);
                
                if ((nameSelector->getCounter() > 0)
                    || m_alwaysDisplayNames) {
                    ClassAndNameHierarchySelectedItem* nameInfo = 
                    new ClassAndNameHierarchySelectedItem(nameSelector);
                    QTreeWidgetItem* nameItem = createTreeWidgetItem(nameSelector->getName(),
                                                                           nameSelector->isSelected(m_displayGroup,
                                                                                                    browserTabIndex),
                                                                           nameInfo);
                    nameTreeWidgets.append(nameItem);
                }
            }
            
            /*
             * Only create a class item if it has valid child names
             */
            if (nameTreeWidgets.empty() == false) {                    
                /*
                 * Item for class
                 */
                ClassAndNameHierarchySelectedItem* classInfo = 
                   new ClassAndNameHierarchySelectedItem(classSelector);
                QTreeWidgetItem* classItem = createTreeWidgetItem(classSelector->getName(),
                                                                        classSelector->isSelected(m_displayGroup,
                                                                                                  browserTabIndex),
                                                                        classInfo);
                classItem->addChildren(nameTreeWidgets);                
                classTreeWidgets.append(classItem);
                classItem->setExpanded(classSelector->isExpanded(m_displayGroup,
                                                                 browserTabIndex));
            }
        }
        
        /*
         * If there are any valid, classes in the model, add a class/name model in the tree widget
         */
        if (classTreeWidgets.empty() == false) {
            ClassAndNameHierarchySelectedItem* modelInfo = 
               new ClassAndNameHierarchySelectedItem(classNamesModel);
            QTreeWidgetItem* modelItem = createTreeWidgetItem(classNamesModel->getName(), 
                                                                    classNamesModel->isSelected(m_displayGroup,
                                                                                                browserTabIndex), 
                                                                    modelInfo);
            modelItem->addChildren(classTreeWidgets);                                   
            m_treeWidget->addTopLevelItem(modelItem);
            modelItem->setExpanded(classNamesModel->isExpanded(m_displayGroup,
                                                               browserTabIndex));
        }
    }
    
    /*
     * Expand collapse items
     */
    const int numTopItems = m_treeWidget->topLevelItemCount();
    for (int nti = 0; nti < numTopItems; nti++) {
        QTreeWidgetItem* twi = m_treeWidget->topLevelItem(nti);
        expandCollapseTreeWidgetItem(twi);
        const int numChildren = twi->childCount();
        for (int ic = 0; ic < numChildren; ic++) {
            QTreeWidgetItem* child = twi->child(ic);
            expandCollapseTreeWidgetItem(child);
        }
    }

    /*
     * File Open, Class Closed
     */
//    m_treeWidget->collapseAll();
//    m_treeWidget->expandToDepth(0);
    
//    m_treeWidget->expandAll();
    
    m_treeWidget->blockSignals(false);
}

/**
 * Expand or collapse tree widget item using its assigned model's status.
 * @param item
 *    The tree widget item.
 */
void 
ClassAndNameHierarchyViewController::expandCollapseTreeWidgetItem(QTreeWidgetItem* item)
{
    void* ptr = item->data(0, Qt::UserRole).value<void*>();
    ClassAndNameHierarchySelectedItem* selectionInfo = (ClassAndNameHierarchySelectedItem*)ptr;
    CaretAssert(selectionInfo);
    
    BrowserTabContent* browserTabContent = 
    GuiManager::get()->getBrowserTabContentForBrowserWindow(m_browserWindowIndex, false);
    const int32_t browserTabIndex = browserTabContent->getTabNumber();
//    DisplayPropertiesBorders* displayPropertiesBorders = GuiManager::get()->getBrain()->getDisplayPropertiesBorders();
//    DisplayGroupEnum::Enum displayGroup = displayPropertiesBorders->getDisplayGroupForTab(browserTabIndex);
    
    switch (selectionInfo->getItemType()) {
        case ClassAndNameHierarchySelectedItem::ITEM_TYPE_HIERARCHY_MODEL:
        {
            ClassAndNameHierarchyModel* hierarchyModel = selectionInfo->getClassAndNameHierarchyModel();
            CaretAssert(hierarchyModel);
            item->setExpanded(hierarchyModel->isExpanded(m_displayGroup,
                                                         browserTabIndex));
        }
            break;
        case ClassAndNameHierarchySelectedItem::ITEM_TYPE_CLASS:
        {
            ClassAndNameHierarchyModel::ClassDisplayGroupSelector* classSelector = selectionInfo->getClassDisplayGroupSelector();
            CaretAssert(classSelector);
            item->setExpanded(classSelector->isExpanded(m_displayGroup,
                                                        browserTabIndex));
        }
            break;
        case ClassAndNameHierarchySelectedItem::ITEM_TYPE_NAME:
            break;
    }
}


/**
 * Create a tree widget item that is checkable.
 * @param text
 *    Text displayed inh item.
 * @param isSelected
 *    True if item is selected.
 */
QTreeWidgetItem*
ClassAndNameHierarchyViewController::createTreeWidgetItem(const AString& name,
                                                          const bool isSelected,
                                                          ClassAndNameHierarchySelectedItem* selectionInfo)
{
    QTreeWidgetItem* twi = new QTreeWidgetItem();
    twi->setText(0, name);
    twi->setFlags(Qt::ItemIsUserCheckable
                  | Qt::ItemIsEnabled);
    if (isSelected) {
        twi->setCheckState(0, Qt::Checked);
    }
    else {
        twi->setCheckState(0, Qt::Unchecked);
    }
    twi->setData(0, Qt::UserRole, qVariantFromValue((void*)selectionInfo));
    
    m_itemSelectionInfo.push_back(selectionInfo);
    
    return twi;
}

