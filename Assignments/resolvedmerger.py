import os
import shutil
import pandas as pd
import re

# -----------------------------
# FOLDERS
# -----------------------------
src_folder = r"C:\Users\dedhi\OneDrive\Desktop\Project\Resolved_Trades_Attempt"
dest_folder = r"C:\Users\dedhi\OneDrive\Desktop\Project\All_Resolved"
merged_output = r"C:\Users\dedhi\OneDrive\Desktop\Project\All_Resolved\All_Resolved_Merged.xlsx"

os.makedirs(dest_folder, exist_ok=True)

resolved_files = []

# -----------------------------
# STEP 1: FIND & COPY FILES
# -----------------------------
for folder_path, subfolders, files in os.walk(src_folder):
    for file in files:
        if file.split(".")[0].endswith("_Resolved"):
            full_path = os.path.join(folder_path, file)
            resolved_files.append(full_path)
            shutil.copy(full_path, dest_folder)

print(f"Found and copied {len(resolved_files)} files.\n")


# -----------------------------
# FUNCTION: CLEAN + SMART SHEET NAME
# -----------------------------
def clean_sheet_name(filename):
    base = os.path.splitext(os.path.basename(filename))[0]
    base = base.replace("_Resolved", "")
    parts = base.split("_")

    try:
        direction = parts[0]
        date = parts[1]
        time = parts[2]

        mmdd = date[4:]
        hhmm = time[:4]

        sheet_name = f"{direction}_{mmdd}_{hhmm}"

    except:
        sheet_name = base.replace("Merged", "")

    sheet_name = re.sub(r'[\[\]\*\:\?\/\\]', '', sheet_name)
    return sheet_name[:31]


# -----------------------------
# STEP 2: CREATE EXCEL WITH SEPARATE CLEAN-NAME SHEETS
# -----------------------------
with pd.ExcelWriter(merged_output, engine="openpyxl") as writer:

    sheet_names = []

    for file in resolved_files:
        ext = file.split(".")[-1].lower()

        sheet_name = clean_sheet_name(file)

        # Ensure unique sheet names
        original = sheet_name
        counter = 1
        while sheet_name in sheet_names:
            sheet_name = f"{original}_{counter}"
            sheet_name = sheet_name[:31]
            counter += 1

        sheet_names.append(sheet_name)

        try:
            if ext == "csv":
                df = pd.read_csv(file)
            elif ext in ["xlsx", "xls"]:
                df = pd.read_excel(file)
            else:
                print(f"Skipping unsupported file: {file}")
                continue

            df.to_excel(writer, index=False, sheet_name=sheet_name)
            print(f"Added sheet: {sheet_name}")

        except Exception as e:
            print(f"Error reading {file}: {e}")


print("\n=======================================")
print(" Completed! Each file added with clean sheet names.")
print(" Saved as:", merged_output)
print(" Total sheets:", len(sheet_names))
print("=======================================")
