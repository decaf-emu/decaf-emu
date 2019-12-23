#pragma once
#include <atomic>
#include <QDirIterator>
#include <QDomDocument>
#include <QPixmap>
#include <QObject>

#include "titlelistmodel.h"
#include "tgahandler.h"

class TitleScanner : public QObject
{
   Q_OBJECT

public:
   void cancel()
   {
      mCancel.store(true);
   }

   void scanDirectoryList(QStringList directories)
   {
      for (auto scanDirectory : directories) {
         auto itr = QDirIterator {
            scanDirectory,
            { "*.rpx" },
            QDir::Files | QDir::Readable, QDirIterator::Subdirectories
         };

         if (mCancel.load()) {
            break;
         }

         while (itr.hasNext() && !mCancel.load()) {
            auto titleInfo = new TitleInfo { };
            auto rpx = QFileInfo { itr.next() };
            auto codeDirectory = QDir { rpx.path() };
            titleInfo->code_path = codeDirectory.path();
            titleInfo->argstr = rpx.fileName();

            auto appXml = QFile { codeDirectory.filePath("app.xml") };
            if (appXml.open(QIODevice::ReadOnly)) {
               auto document = QDomDocument { };
               if (document.setContent(&appXml)) {
                  auto app = document.documentElement();
                  if (app.tagName() == "app") {
                     titleInfo->title_id = app.firstChildElement("title_id").text().trimmed().toULongLong(nullptr, 16);
                     titleInfo->title_version = app.firstChildElement("title_version").text().trimmed().toInt(nullptr, 16);
                  }
               }
            }

            auto cosXml = QFile { codeDirectory.filePath("cos.xml") };
            if (cosXml.open(QIODevice::ReadOnly)) {
               auto document = QDomDocument { };
               if (document.setContent(&cosXml)) {
                  auto cos = document.documentElement();
                  if (cos.tagName() == "app") {
                     titleInfo->argstr = cos.firstChildElement("argstr").text().trimmed();
                  }
               }
            }

            auto metaDirectory = QDir { codeDirectory.filePath("../meta") };
            if (metaDirectory.exists()) {
               auto metaXml = QFile { metaDirectory.filePath("meta.xml") };
               if (metaXml.open(QIODevice::ReadOnly)) {
                  auto document = QDomDocument { };
                  if (document.setContent(&metaXml)) {
                     auto meta = document.documentElement();
                     if (meta.tagName() == "menu") {
                        titleInfo->longname_en = meta.firstChildElement("longname_en").text().trimmed();
                        titleInfo->publisher_en = meta.firstChildElement("publisher_en").text().trimmed();
                     }
                  }
               }

               auto iconTex = QFile { metaDirectory.filePath("iconTex.tga") };
               if (iconTex.open(QIODevice::ReadOnly)) {
                  auto tgaHandler = TGAHandler { };
                  auto image = QImage { };
                  tgaHandler.setDevice(&iconTex);
                  if (tgaHandler.read(&image)) {
                     titleInfo->icon = QPixmap::fromImage(image);
                  }
               }
            }

            emit titleFound(titleInfo);
         }
      }

      mCancel = false;
      emit scanFinished();
   }

signals:
   void titleFound(TitleInfo *info);
   void scanFinished();

private:
   std::atomic_bool mCancel { false };
};
