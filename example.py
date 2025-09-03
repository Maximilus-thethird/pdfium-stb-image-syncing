import example_wrapper
import numpy as np

def load_path_as_numpy(path):
    arr = None
    try:
        if path.lower().endswith(".jpg"):
            arr = example_wrapper.render_image(path, 224, 224)
        elif path.lower().endswith(".pdf"):
            arr_bgra = example_wrapper.render_page(path, 0, 224, 224, 0)
            arr = arr_bgra[:, :, [2, 1, 0]]
    except FileNotFoundError:
        print("Can't open Image")
        return None
    return arr

np_arr = load_path_as_numpy(r"D:\DEVELOPMENTS\dummy_pdf_for_training\layer0\TEST CT\2.jpg")
reshaped = np_arr.reshape(np_arr.shape[0], -1)
np.savetxt('img_array.txt', reshaped, delimiter=',', fmt='%.2f')

#D:/DEVELOPMENTS/dummy_pdf_for_training\layer0\TEST CT\742072001 Function GO Gauge 2 for Stator B (G20140032) - Copy (2) - Copy - Copy - Copy.pdf
#D:\DEVELOPMENTS\dummy_pdf_for_training\layer0\TEST CT\2.jpg