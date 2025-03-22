#pragma once
#include <wx/wx.h>
#include <opencv2/opencv.hpp>
#include <wx/timer.h>
#include <wx/thread.h>
#include <wx/filedlg.h>
#include <wx/listctrl.h>
#include <wx/zipstrm.h>
#include <wx/wfstream.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <vector>

class MainFrame : public wxFrame
{
public:
    MainFrame(const wxString& title);
    ~MainFrame();

private:
    // Boutons
    wxButton* m_captureButton;
    wxButton* m_snapshotButton;
    wxButton* m_xProfileButton;
    wxButton* m_saveZipButton;  // Bouton pour sauvegarder les images en ZIP

    wxStaticText* m_captureText;
    wxStaticText* m_snapshotText;
    wxStaticText* m_xProfileText;

    // Éléments pour la vidéo
    wxPanel* m_videoPanel;
    wxTimer* m_timer;
    cv::VideoCapture m_capture;
    bool m_isCapturing;

    // Pour stocker la frame courante
    cv::Mat m_currentFrame;
    wxMutex m_frameMutex;

    // Pour stocker les snapshots
    std::vector<cv::Mat> m_snapshots;
    wxListCtrl* m_snapshotList;  // Liste pour afficher les snapshots pris

    // Méthodes
    void OnTimer(wxTimerEvent& event);
    void OnPaint(wxPaintEvent& event);
    void OnCaptureButton(wxCommandEvent& event);
    void OnSnapshotButton(wxCommandEvent& event);
    void OnSaveZipButton(wxCommandEvent& event);
    void OnXProfileButton(wxCommandEvent& event);
    void OnSnapshotListItemActivated(wxListEvent& event);

    // Convertir une image OpenCV en wxImage
    wxImage ConvertOpenCVToWxImage(const cv::Mat& image);

    // Sauvegarder les images dans un fichier ZIP
    bool SaveImagesToZip(const std::vector<cv::Mat>& images, const wxString& zipPath);

    DECLARE_EVENT_TABLE()
};