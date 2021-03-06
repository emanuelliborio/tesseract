/**********************************************************************
 * File:        lstmboxrenderer.cpp
 * Description: Renderer for creating box file for LSTM training.
 *              based on the tsv renderer.
 *
 * (C) Copyright 2006, Google Inc.
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 ** http://www.apache.org/licenses/LICENSE-2.0
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 *
 **********************************************************************/

#include "baseapi.h"  // for TessBaseAPI
#include "renderer.h"
#include "tesseractclass.h"  // for Tesseract

namespace tesseract {

/**
 * Create a UTF8 box file for LSTM training from the internal data structures.
 * page_number is a 0-base page index that will appear in the box file.
 * Returned string must be freed with the delete [] operator.
 */
static void AddBoxToLSTM(int right, int bottom, int top,
                        int image_height_, int page_num,
                        STRING* text) {
        text->add_str_int(" ", image_height_ - bottom);
        text->add_str_int(" ", right + 5);
        text->add_str_int(" ", image_height_ - top);
        text->add_str_int(" ", page_num);
}

char* TessBaseAPI::GetLSTMBOXText(int page_number) {
  if (tesseract_ == nullptr || (page_res_ == nullptr && Recognize(nullptr) < 0))
    return nullptr;

  STRING lstm_box_str("");
  int page_num = page_number;
  bool first_word = true;
  int left, top, right, bottom;

  LTRResultIterator* res_it = GetLTRIterator();
  while (!res_it->Empty(RIL_BLOCK)) {
    if (res_it->Empty(RIL_SYMBOL)) {
      res_it->Next(RIL_SYMBOL);
      continue;
    }
  if (!first_word) {
   if (!(res_it->IsAtBeginningOf(RIL_TEXTLINE))) {
    if (res_it->IsAtBeginningOf(RIL_WORD)) {
        lstm_box_str.add_str_int("  ", left);
        AddBoxToLSTM(right, bottom, top, image_height_, page_num, &lstm_box_str);
        lstm_box_str += "\n";  // end of row for word
    } // word
   } else {
    if (res_it->IsAtBeginningOf(RIL_TEXTLINE)) {
        lstm_box_str.add_str_int("\t ", left);
        AddBoxToLSTM(right, bottom, top, image_height_, page_num, &lstm_box_str);
        lstm_box_str += "\n";  // end of row for line
    } // line
   }
  } // not first word
  first_word=false;
  // Use bounding box for whole line for everything
  res_it->BoundingBox(RIL_TEXTLINE, &left, &top, &right, &bottom);
  do { lstm_box_str +=
                 std::unique_ptr<const char[]>(res_it->GetUTF8Text(RIL_SYMBOL)).get();
        res_it->Next(RIL_SYMBOL);
     } while (!res_it->Empty(RIL_BLOCK) && !res_it->IsAtBeginningOf(RIL_SYMBOL));
    lstm_box_str.add_str_int(" ", left);
    AddBoxToLSTM(right, bottom, top, image_height_, page_num, &lstm_box_str);
    lstm_box_str += "\n";  // end of row for symbol
  }
  lstm_box_str.add_str_int("\t ", left);
  AddBoxToLSTM(right, bottom, top, image_height_, page_num, &lstm_box_str);
  lstm_box_str += "\n";  // end of PAGE
  char* ret = new char[lstm_box_str.length() + 1];
  strcpy(ret, lstm_box_str.string());
  delete res_it;
  return ret;
}

/**********************************************************************
 * LSTMBOX Renderer interface implementation
 **********************************************************************/
TessLSTMBOXRenderer::TessLSTMBOXRenderer(const char *outputbase)
    : TessResultRenderer(outputbase, "box") {
}

bool TessLSTMBOXRenderer::AddImageHandler(TessBaseAPI* api) {
  const std::unique_ptr<const char[]> lstmbox(api->GetLSTMBOXText(imagenum()));
  if (lstmbox == nullptr) return false;

  AppendString(lstmbox.get());

  return true;
}

}  // namespace tesseract.
