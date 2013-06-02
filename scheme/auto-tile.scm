;;; Autotiling routines for nwm
;;;
;;; Copyright (C) 2013  Brandon Invergo
;;; Copyright (C) 2010-2012  Nathan Sullivan
;;;
;;; This program is free software; you can redistribute it and/or 
;;; modify it under the terms of the GNU General Public License 
;;; as published by the Free Software Foundation; either version 2 
;;; of the License, or (at your option) any later version. 
;;;
;;; This program is distributed in the hope that it will be useful, 
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of 
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
;;; GNU General Public License for more details. 
;;;
;;; You should have received a copy of the GNU General Public License 
;;; along with this program; if not, write to the Free Software 
;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  
;;; 02110-1301, USA 
;;;

; number of "master" windows
(define master-count 1)

; the percent of the screen width dedicated to the master windows
(define master-perc 50)

(define (arrange-client client x y width height)
  (move-client client x y)
  (resize-client client width height))

(define (split-vertical-iter clients x width increment cur gap)
  (arrange-client (car clients) x (+ cur gap) width (- increment (* 2 gap)))
  (if (= (length clients) 1)
      #t
      (split-vertical-iter (cdr clients) x width increment (+ cur increment) gap)))

(define (split-vertical clients x width gap)
  (if (> (length clients) 0)
      (let ((increment (floor (/ (screen-height) (length clients)))))
        (split-vertical-iter clients x width increment 0 gap))))

(define (split-horizontal-iter clients y height increment cur gap)
  (arrange-client (car clients) (+ cur gap) y (- increment (* 2 gap)) height)
  (if (= (length clients) 1)
      #t
      (split-horizontal-iter (cdr clients) y height increment (+ cur increment) gap)))

(define (split-horizontal clients y height gap)
  (if (> (length clients) 0)
      (let ((increment (floor (/ (screen-width) (length clients)))))
        (split-horizontal-iter clients y height increment 0 gap))))

(define (auto-vtile gap)
  (let* ((clients (all-clients))
        (client-count (length clients))
        (master-screen-width (floor (* (screen-width) (/ master-perc 100)))))
    (cond
     ((= client-count 1) (arrange-client (car clients)
                                         gap gap
                                         (- (screen-width) (* 2 gap))
                                         (- (screen-height) (* 2 gap))))
     ((> client-count 1)
      (split-vertical
       (list-head clients (min client-count master-count))
       gap (- master-screen-width (* 2 gap)) gap)
      (split-vertical
       (list-tail clients (min client-count master-count))
       (+ master-screen-width gap) (- (- (screen-width) master-screen-width) (* 2 gap)) gap)))))

(define (auto-htile gap)
  (let* ((clients (all-clients))
        (client-count (length clients))
        (master-screen-height (floor (* (screen-height) (/ master-perc 100)))))
    (cond
     ((= client-count 1) (arrange-client (car clients)
                                         gap gap
                                         (- (screen-width) (* 2 gap))
                                         (- (screen-height) (* 2 gap))))
     ((> client-count 1)
      (split-horizontal
       (list-head clients (min client-count master-count))
       gap (- master-screen-height (* 2 gap)) gap)
      (split-horizontal
       (list-tail clients (min client-count master-count))
       (+ master-screen-height gap) (- (- (screen-height) master-screen-height) (* 2 gap)) gap)))))

(define (swap-master)
  (let ((master (car (all-clients)))
        (focused (get-focus-client)))
    (if (equal? master focused)
        (client-list-swap focused (cadr (all-clients)))
        (client-list-swap focused master))
    (focus-client (car (all-clients)))))
