#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "fpdfview.h"

#include <stdexcept>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

namespace py = pybind11;

py::array_t<uint8_t> render_page_helper(FPDF_PAGE page, int target_width = 0, int target_height = 0, int dpi = 80) {
    int width, height;

    if (target_width > 0 && target_height > 0) {
        width = target_width;
        height = target_height;
    } else {
        width = static_cast<int>(FPDF_GetPageWidth(page) * dpi / 72.0);
        height = static_cast<int>(FPDF_GetPageHeight(page) * dpi / 72.0);
    }

    FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 1);
    if (!bitmap) throw std::runtime_error("Failed to create bitmap");

    // FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, 0);

    // int stride = FPDFBitmap_GetStride(bitmap);
    uint8_t* buffer = static_cast<uint8_t*>(FPDFBitmap_GetBuffer(bitmap));

    // Return numpy array with shape (height, width, 4) = BGRA
    auto result = py::array_t<uint8_t>({height, width, 4}, buffer);
    std::memcpy(result.mutable_data(), buffer, height * width * 4);
    FPDFBitmap_Destroy(bitmap);
    return result;
}

py::array_t<uint8_t> render_image(const std::string& filename, int target_width = 224, int target_height = 224) {
    int width, height, channels;
    unsigned char* pixel_data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
    if (!pixel_data) throw std::runtime_error("Failed to load image");

    // Temporary resized buffer with original channels
    std::vector<uint8_t> resized(target_width * target_height * channels);
    stbir_resize_uint8(pixel_data, width, height, 0,
                       resized.data(), target_width, target_height, 0, channels);
    stbi_image_free(pixel_data);

    // Always return RGB (3 channels)
    py::array_t<uint8_t> result({target_height, target_width, 3});
    auto buf = result.mutable_unchecked<3>();

    for (int y = 0; y < target_height; ++y) {
        for (int x = 0; x < target_width; ++x) {
            int idx = (y * target_width + x) * channels;
            buf(y, x, 0) = resized[idx + 0]; // R
            buf(y, x, 1) = resized[idx + 1]; // G
            buf(y, x, 2) = resized[idx + 2]; // B
            // If channels == 1 (grayscale), R=G=B
            if (channels == 1) buf(y, x, 1) = buf(y, x, 2) = buf(y, x, 0);
        }
    }

    return result;
}

py::array_t<uint8_t> render_page(const std::string& filename, int page, int target_width = 0, int target_height = 0, int dpi = 80) {
    FPDF_DOCUMENT doc = FPDF_LoadDocument(filename.c_str(), nullptr);
    if (!doc) throw std::runtime_error("Failed to open PDF");

    FPDF_PAGE fpdf_page = FPDF_LoadPage(doc, page);
    auto arr = render_page_helper(fpdf_page, target_width, target_height, dpi);

    FPDF_ClosePage(fpdf_page);
    FPDF_CloseDocument(doc);
    return arr;
}

PYBIND11_MODULE(example_wrapper, m) {
    FPDF_InitLibrary();
    atexit([](){ FPDF_DestroyLibrary(); });
    m.def("render_page", &render_page, py::arg("filename"),
          py::arg("page"),
          py::arg("target_width") = 0,
          py::arg("target_height") = 0,
          py::arg("dpi") = 80,
          "Render one page of a PDF to numpy arrays (BGRA). "
          "If target_width/height > 0, renders directly at that size.");
    m.def("render_image", &render_image, py::arg("filename"),
          py::arg("target_width") = 0,
          py::arg("target_height") = 0,
          "Convert PIL Image to numpy arrays (BGRA). ");
}