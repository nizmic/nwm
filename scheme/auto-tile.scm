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

;;; This file defines procedures for implementing auto-tiling

;; Back-end window arrangement procedures
; move and resize a client
(define (arrange-client client x y width height)
  (move-client client x y)
  (resize-client client width height))

; create a vertical stack of clients, helper function
(define (split-vertical-iter clients x width increment cur gap)
  (arrange-client (car clients) x (+ cur gap) width (- increment (* 2 gap)))
  (if (= (length clients) 1)
      #t
      (split-vertical-iter (cdr clients) x width increment (+ cur increment) gap)))

; create a vertical stack of clients
(define (split-vertical clients x width gap)
  (if (> (length clients) 0)
      (let ((increment (floor (/ (screen-height) (length clients)))))
        (split-vertical-iter clients x width increment 0 gap))))

; create a horizontal stack of clients, helper function
(define (split-horizontal-iter clients y height increment cur gap)
  (arrange-client (car clients) (+ cur gap) y (- increment (* 2 gap)) height)
  (if (= (length clients) 1)
      #t
      (split-horizontal-iter (cdr clients) y height increment (+ cur increment) gap)))

; create a horizontal stack of clients
(define (split-horizontal clients y height gap)
  (if (> (length clients) 0)
      (let ((increment (floor (/ (screen-width) (length clients)))))
        (split-horizontal-iter clients y height increment 0 gap))))

;; arrangement functions
; vertical tiling: a master area on the left and a vertical stack on the right
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

; horizontal tiling: a master area on the top and a horizontal stack on the bottom
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

;; New hooks
(define auto-tile-hook (make-hook 1))

;; User-facing variables
; number of "master" windows
(define master-count 1)

; the percent of the screen width dedicated to the master windows
(define master-perc 50)

; the size of the gap between windows in pixels (not counting borders)
(define gap 2)

; the size of the borders in pixels
(define border-width 1)

; list of all available arrangements
(define auto-tile-arrangements (list auto-vtile auto-htile))

; the currently used arrangements
(define auto-tile-arrangement (car auto-tile-arrangements))

;; User-facing procedures
; arrange the clients using the current arrangement procedure
(define auto-tile (lambda ()
                    (begin
                      (auto-tile-arrangement gap)
                      (run-hook auto-tile-hook (all-clients)))))

; cycle through the arrangements
(define (auto-tile-cycle-arrangement)
  (begin
    (set! auto-tile-arrangements (append (cdr auto-tile-arrangements)
                                         (list (car auto-tile-arrangements))))
    (set! auto-tile-arrangement (car auto-tile-arrangements))
    (auto-tile)))


; swap the master client with another client
(define (swap-master)
  (let ((master (car (all-clients)))
        (focused (get-focus-client)))
    (if (equal? master focused)
        (client-list-swap focused (cadr (all-clients)))
        (client-list-swap focused master))
    (focus-client (car (all-clients)))))

; add another client to the master area
(define (add-master)
  (set! master-count (+ master-count 1))
  (auto-tile))

; remove a client from the master area
(define (remove-master)
  (if (> master-count 1)
      (set! master-count (- master-count 1)))
  (auto-tile))

; grow the master area
(define (grow-master)
  (if (< master-perc 94)
      (set! master-perc (+ master-perc 5)))
  (auto-tile))

; shrink the master area
(define (shrink-master)
  (if (>= master-perc 6)
      (set! master-perc (- master-perc 5)))
  (auto-tile))

;; Set up hooks
; map-client
(add-hook! map-client-hook focus-client)
(add-hook! map-client-hook (lambda (client)
                             (auto-tile)))

; unmap-client
(add-hook! unmap-client-hook (lambda (client)
                               (focus-client (next-client client))))
(add-hook! unmap-client-hook (lambda (client)
                               (auto-tile)))

; destroy-client
(add-hook! destroy-client-hook (lambda (client)
                                 (focus-client (next-client client))))
(add-hook! destroy-client-hook (lambda (client)
                                 (auto-tile)))

;; Default keybindings
; add a master, mod4-i
(bind-key 64 "i" add-master)

; remove a master, mod4-d
(bind-key 64 "d" remove-master)

; grow master, mod4-l
(bind-key 64 "l" grow-master)

; shrink master, mod4-h
(bind-key 64 "h" shrink-master)

; cycle arrangements, mod4-shift-space
(bind-key 65 "Space" auto-tile-cycle-arrangement)

; swap the focused window into master
(bind-key 64 "s" (lambda ()
                   (begin
                     (swap-master)
                     (auto-tile))))
