;;; Sample init.scm file for nwm
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

; auto-tile options
(load "auto-tile.scm")
(define master-count 1)
(define master-width 50)
(define border-width 1)
(define gap 2)

; load window tagging
(load "tags.scm")

; commands
; path to terminal program
(define term-program '("xterm"))
; you can supply arguments like so:
; (define term-program '("xterm" "-e" "screen"))

; window borders
(define norm-border-color #x2b2b2b)
(define focus-border-color #x6CA0A3)

(define (draw-norm-borders client-list color)
  (if (null? client-list)
      (clear)
      (begin
        (draw-norm-borders (cdr client-list) color)
        (draw-border (car client-list) color border-width))))

(define (draw-focus-border client color)
  (draw-border client color border-width))

(define (draw-borders client-list norm-color focus-color)
  (let ((focused (get-focus-client)))
    (begin
      (draw-norm-borders client-list norm-color)
      (if (not (unspecified? focused))
          (draw-focus-border focused focus-color)))))

(define (focus-next)
  (let ((focused (get-focus-client))
        (visible (visible-clients)))
    (if (not (null? visible))
        (if (unspecified? focused)
            (focus-client (car visible))
            (focus-client (next-client focused))))))

(define (focus-prev)
  (let ((focused (get-focus-client))
        (visible (visible-clients)))
    (if (not (null? visible))
        (if (unspecified? focused)
            (focus-client (car visible))
            (focus-client (prev-client focused))))))

(define (close)
  (destroy-client (get-focus-client)))

(define (launch-term)
  (launch-program term-program))

; focus next, ctrl-j
(bind-key 4 "j" focus-next)

; focus prev, ctrl-k
(bind-key 4 "k" focus-prev)

; grow master, ctrl-l
(bind-key 4 "l" (lambda ()
                  (grow-master 5)))

; shrink master, ctrl-h
(bind-key 4 "h" (lambda ()
                  (shrink-master 5)))

; launch terminal, ctrl-enter
(bind-key 4 "Enter" launch-term)

(bind-key 4 "s" (lambda ()
                   (begin
                     (swap-master)
                     (auto-tile (visible-clients)))))

; close window, ctrl-shift-c
(bind-key 5 "c" close)

; hooks
; redraw window borders upon focus change
(add-hook! focus-client-hook (lambda (client)
                               (draw-borders (visible-clients) norm-border-color
                                             focus-border-color)))
; redraw borders upon auto-tiling
(add-hook! auto-tile-hook (lambda (clients)
                            (draw-borders clients norm-border-color
                                          focus-border-color)))
